//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.md file in the project root for full license information.
//

#include "stdafx.h"
#include "MLFDataDeserializer.h"
#include "ConfigHelper.h"
#include "htkfeatio.h"
#include "msra_mgram.h"
#include "ElementTypeUtils.h"

namespace Microsoft { namespace MSR { namespace CNTK {

static float s_oneFloat = 1.0;
static double s_oneDouble = 1.0;

struct MLFDataDeserializer::MLFUtterance : SequenceDescription
{
    size_t sequenceStart;
    size_t frameStart;
};

struct MLFDataDeserializer::MLFFrame : SequenceDescription
{
    size_t index;
};

// Currently we only have a single mlf chunk that contains a vector of all labels.
// TODO: This will be changed in the future to work only on a subset of chunks 
// at each point in time.
class MLFDataDeserializer::MLFChunk : public Chunk
{
    MLFDataDeserializer* m_parent;
public:
    MLFChunk(MLFDataDeserializer* parent) : m_parent(parent)
    {}

    virtual std::vector<SequenceDataPtr> GetSequence(const size_t& sequenceId) override
    {
        return m_parent->GetSequenceById(sequenceId);
    }
};

MLFDataDeserializer::MLFDataDeserializer(CorpusDescriptorPtr corpus, const ConfigParameters& label, const std::wstring& name)
{
    bool frameMode = label.Find("frameMode", "true");
    if (!frameMode)
    {
        LogicError("Currently only reader only supports fram mode. Please check your configuration.");
    }

    ConfigHelper::CheckLabelType(label);
    size_t dimension = ConfigHelper::GetLabelDimension(label);

    // TODO: Similarly to the old reader, currently we assume all Mlfs will have same root name (key)
    // restrict MLF reader to these files--will make stuff much faster without having to use shortened input files

    // TODO: currently we do not use symbol and word tables.
    const msra::lm::CSymbolSet* wordTable = nullptr;
    std::map<string, size_t>* symbolTable = nullptr;
    std::vector<std::wstring> mlfPaths = ConfigHelper::GetMlfPaths(label);
    std::wstring stateListPath = static_cast<std::wstring>(label(L"labelMappingFile", L""));
    const double htkTimeToFrame = 100000.0; // default is 10ms
    msra::asr::htkmlfreader<msra::asr::htkmlfentry, msra::lattices::lattice::htkmlfwordsequence> labels(mlfPaths, std::set<wstring>(), stateListPath, wordTable, symbolTable, htkTimeToFrame);

    // Make sure 'msra::asr::htkmlfreader' type has a move constructor
    static_assert(
        std::is_move_constructible<
            msra::asr::htkmlfreader<msra::asr::htkmlfentry,
                                    msra::lattices::lattice::htkmlfwordsequence>>::value,
        "Type 'msra::asr::htkmlfreader' should be move constructible!");

    m_elementType = ConfigHelper::GetElementType(label);

    MLFUtterance description;
    description.m_isValid = true;
    size_t totalFrames = 0;

    for (const auto& l : labels)
    {
        description.m_key.major = l.first;
        const auto& utterance = l.second;
        description.sequenceStart = m_classIds.size();
        description.m_isValid = true;
        size_t numberOfFrames = 0;

        foreach_index(i, utterance)
        {
            const auto& timespan = utterance[i];
            if ((i == 0 && timespan.firstframe != 0) ||
                (i > 0 && utterance[i - 1].firstframe + utterance[i - 1].numframes != timespan.firstframe))
            {
                RuntimeError("Labels are not in the consecutive order MLF in label set: %ls", l.first.c_str());
            }

            if (timespan.classid >= dimension)
            {
                RuntimeError("Class id %d exceeds the model output dimension %d.", (int)timespan.classid,(int)dimension);
            }

            if (timespan.classid != static_cast<msra::dbn::CLASSIDTYPE>(timespan.classid))
            {
                RuntimeError("CLASSIDTYPE has too few bits");
            }

            for (size_t t = timespan.firstframe; t < timespan.firstframe + timespan.numframes; t++)
            {
                m_classIds.push_back(timespan.classid);
                numberOfFrames++;
            }
        }

        description.m_numberOfSamples = numberOfFrames;
        totalFrames += numberOfFrames;
        m_utteranceIndex.push_back(m_frames.size());
        m_keyToSequence[description.m_key.major] = m_utteranceIndex.size() - 1;

        MLFFrame f;
        f.m_chunkId = 0;
        f.m_numberOfSamples = 1;
        f.m_key.major = description.m_key.major;
        f.m_isValid = description.m_isValid;
        for (size_t k = 0; k < description.m_numberOfSamples; ++k)
        {
            f.m_id = m_frames.size();
            f.m_key.minor = k;
            f.index = description.sequenceStart + k;
            m_frames.push_back(f);
            m_sequences.push_back(&m_frames[f.m_id]);
        }
    }

    m_sequences.reserve(m_frames.size());
    for (int i = 0; i < m_frames.size(); ++i)
    {
        m_sequences.push_back(&m_frames[i]);
    }

    StreamDescriptionPtr stream = std::make_shared<StreamDescription>();
    stream->m_id = 0;
    stream->m_name = name;
    stream->m_sampleLayout = std::make_shared<TensorShape>(dimension);
    stream->m_storageType = StorageType::sparse_csc;
    stream->m_elementType = m_elementType;
    m_streams.push_back(stream);
}

const SequenceDescriptions& MLFDataDeserializer::GetSequenceDescriptions() const
{
    return m_sequences;
}

std::vector<StreamDescriptionPtr> MLFDataDeserializer::GetStreamDescriptions() const
{
    return m_streams;
}

ChunkPtr MLFDataDeserializer::GetChunk(size_t chunkId)
{
    UNUSED(chunkId);
    assert(chunkId == 0);
    return std::make_shared<MLFChunk>(this);
}

std::vector<SequenceDataPtr> MLFDataDeserializer::GetSequenceById(size_t sequenceId)
{
    size_t label = m_classIds[m_frames[sequenceId].index];
    SparseSequenceDataPtr r = std::make_shared<SparseSequenceData>();
    r->m_indices.resize(1);
    r->m_indices[0] = std::vector<size_t>{ label };

    if (m_elementType == ElementType::tfloat)
    {
        r->m_data = &s_oneFloat;
    }
    else
    {
        assert(m_elementType == ElementType::tdouble);
        r->m_data = &s_oneDouble;
    }

    return std::vector<SequenceDataPtr> { r };
}

const SequenceDescription* MLFDataDeserializer::GetSequenceDescriptionByKey(const KeyType& key)
{
    size_t sequenceId = m_keyToSequence[key.major];
    size_t index = m_utteranceIndex[sequenceId] + key.minor;
    return m_sequences[index];
}

}}}
