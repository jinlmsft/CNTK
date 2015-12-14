#include "stdafx.h"
#include "MLFDataDeserializer.h"
#include "ConfigHelper.h"
#include "htkfeatio.h"
#include "msra_mgram.h"

namespace Microsoft { namespace MSR { namespace CNTK {

    MLFDataDeserializer::MLFDataDeserializer(const ConfigParameters& label)
        : m_mlfPaths(std::move(ConfigHelper::GetMlfPaths(label)))
    {
        ConfigHelper::CheckLabelType(label);

        m_dimension = ConfigHelper::GetLabelDimension(label);
        m_layout = std::make_shared<ImageLayout>(std::move(std::vector<size_t> { m_dimension }));

        m_stateListPath = label(L"labelMappingFile", L"");

        // TODO: currently assumes all Mlfs will have same root name (key)
        // restrict MLF reader to these files--will make stuff much faster without having to use shortened input files

        // get labels
        double htktimetoframe = 100000.0; // default is 10ms

        const msra::lm::CSymbolSet* wordTable = nullptr;
        std::map<string, size_t>* symbolTable = nullptr;

        msra::asr::htkmlfreader<msra::asr::htkmlfentry, msra::lattices::lattice::htkmlfwordsequence> labels
            (m_mlfPaths, std::set<wstring>(), m_stateListPath, wordTable, symbolTable, htktimetoframe);

        // Make sure 'msra::asr::htkmlfreader' type has a move constructor
         static_assert(
             std::is_move_constructible<
                msra::asr::htkmlfreader<msra::asr::htkmlfentry,
                msra::lattices::lattice::htkmlfwordsequence>>::value,
             "Type 'msra::asr::htkmlfreader' should be move constructible!");
         /*
         for (auto l : labels)
         {
             const auto & labseq = l.second;
             foreach_index(i, labseq)
             {
                 const auto & e = labseq[i];
                 if ((i == 0 && e.firstframe != 0) || (i > 0 && labseq[i - 1].firstframe + labseq[i - 1].numframes != e.firstframe))
                 {
                     RuntimeError("minibatchutterancesource: labels not in consecutive order MLF in label set: %ls", l.first.c_str());
                 }

                 auto dimension = m_dimension;
                 if (e.classid >= dimension)
                 {
                     RuntimeError("minibatchutterancesource: class id %llu exceeds model output dimension %llu in file %ls", e.classid, dimension, l.first.c_str());
                 }

                 if (e.classid != (msra::dbn::CLASSIDTYPE)e.classid)
                 {
                     RuntimeError("CLASSIDTYPE has too few bits");
                 }

                 for (size_t t = e.firstframe; t < e.firstframe + e.numframes; t++)
                 {
                     m_classids[j]->push_back(e.classid);
                 }

                 numclasses[j] = max(numclasses[j], (size_t)(1u + e.classid));
             }

             m_classids[j]->push_back((msra::dbn::CLASSIDTYPE) - 1);  // append a boundary marker marker for checking

             if (!labels[j].empty() && m_classids[j]->size() != m_totalframes + utteranceset.size())
                 LogicError("minibatchutterancesource: label duration inconsistent with feature file in MLF label set: %ls", key.c_str());
             assert(labels[j].empty() || m_classids[j]->size() == m_totalframes + utteranceset.size());
         }*/
    }

    void Microsoft::MSR::CNTK::MLFDataDeserializer::SetEpochConfiguration(const EpochConfiguration& /*config*/)
    {
        throw std::logic_error("The method or operation is not implemented.");
    }

    TimelineP Microsoft::MSR::CNTK::MLFDataDeserializer::GetSequenceDescriptions() const
    {
        throw std::logic_error("The method or operation is not implemented.");
    }

    Microsoft::MSR::CNTK::InputDescriptionPtr Microsoft::MSR::CNTK::MLFDataDeserializer::GetInput() const
    {
        throw std::logic_error("The method or operation is not implemented.");
    }

    Microsoft::MSR::CNTK::Sequence Microsoft::MSR::CNTK::MLFDataDeserializer::GetSequenceById(size_t /*id*/)
    {
        throw std::logic_error("The method or operation is not implemented.");
    }

    Microsoft::MSR::CNTK::Sequence Microsoft::MSR::CNTK::MLFDataDeserializer::GetSampleById(size_t /*sequenceId*/, size_t /*sampleId*/)
    {
        throw std::logic_error("The method or operation is not implemented.");
    }

    bool Microsoft::MSR::CNTK::MLFDataDeserializer::RequireChunk(size_t /*chunkIndex*/)
    {
        throw std::logic_error("The method or operation is not implemented.");
    }

    void Microsoft::MSR::CNTK::MLFDataDeserializer::ReleaseChunk(size_t /*chunkIndex*/)
    {
        throw std::logic_error("The method or operation is not implemented.");
    }
}}}