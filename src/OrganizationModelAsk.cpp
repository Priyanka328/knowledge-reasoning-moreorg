#include "OrganizationModelAsk.hpp"
#include "Algebra.hpp"
#include <sstream>
#include <base/Logging.hpp>
#include <base/Time.hpp>
#include <numeric/LimitedCombination.hpp>
#include <owlapi/model/OWLOntologyAsk.hpp>
#include <owlapi/model/OWLOntologyTell.hpp>
#include <owlapi/Vocabulary.hpp>
#include <organization_model/reasoning/ResourceMatch.hpp>
#include <organization_model/vocabularies/OM.hpp>


using namespace owlapi::model;
using namespace owlapi::vocabulary;

namespace organization_model {

OrganizationModelAsk::OrganizationModelAsk(const OrganizationModel::Ptr& om,
        const ModelPool& modelPool,
        bool applyFunctionalSaturationBound)
    : mpOrganizationModel(om)
    , mOntologyAsk(om->ontology())
    , mApplyFunctionalSaturationBound(applyFunctionalSaturationBound)
{
    if(!modelPool.empty())
    {
        if(!mApplyFunctionalSaturationBound)
        {
            LOG_INFO_S << "No functional saturation bound requested: this might take some time to prepare the functionality mappings";
        }
        prepare(modelPool, mApplyFunctionalSaturationBound);
    } else {
        LOG_INFO_S << "No model pool provided: did not prepare functionality mappings";
    }
}

void OrganizationModelAsk::prepare(const ModelPool& modelPool, bool applyFunctionalSaturationBound)
{
    mModelPool = modelPool;
    mFunctionalityMapping = getFunctionalityMapping(mModelPool, applyFunctionalSaturationBound);
}

owlapi::model::IRIList OrganizationModelAsk::getServiceModels() const
{
    bool directSubclassOnly = false;
    IRIList subclasses = mOntologyAsk.allSubClassesOf(vocabulary::OM::Service(), directSubclassOnly);
    return subclasses;
}

owlapi::model::IRIList OrganizationModelAsk::getFunctionalities() const
{
    bool directSubclassOnly = false;
    IRIList subclasses = mOntologyAsk.allSubClassesOf(vocabulary::OM::Functionality(), directSubclassOnly);
    LOG_DEBUG_S << "Functionalities: " << subclasses;
    return subclasses;
}

FunctionalityMapping OrganizationModelAsk::getFunctionalityMapping(const ModelPool& modelPool, bool applyFunctionalSaturationBound) const
{
    if(modelPool.empty())
    {
        throw std::invalid_argument("organization_model::OrganizationModel::getFunctionalityMaps"
                " cannot compute functionality map for empty model pool");
    }

    IRIList functionalityModels = getFunctionalities();
    std::pair<Pool2FunctionMap, Function2PoolMap> functionalityMaps;

    ModelPool functionalSaturationBound;
    // TODO: create an bound on the model pool based on the given service
    // models based on the FunctionalSaturation -- avoids computing unnecessary
    // combination as function mappings
    //
    // LocationImageProvider: CREX --> 1, Sherpa --> 1
    if(applyFunctionalSaturationBound)
    {
        // Compute service set of all known functionalities
        FunctionalitySet functionalities = Functionality::toFunctionalitySet(functionalityModels);

        // Compute the bound for all services
        LOG_DEBUG_S << "Get functional saturation bound for '" << functionalityModels;
        functionalSaturationBound = getFunctionalSaturationBound(functionalities);
        LOG_DEBUG_S << "Functional saturation bound for '" << functionalityModels << "' is "
            << functionalSaturationBound.toString();

        // Apply bound to the existing model pool
        functionalSaturationBound = modelPool.applyUpperBound(functionalSaturationBound);
        LOG_INFO_S << "Model pool after applying the functional saturation bound for '" << functionalityModels << "' is "
            << functionalSaturationBound.toString();
    } else {
        LOG_DEBUG_S << "No functional saturation bound applied (since bounding has not been requested)";
        functionalSaturationBound = modelPool;
    }

    if(modelPool.empty())
    {
        throw std::runtime_error("organization_model::OrganizationModelAsk::getFunctionalityMapping: provided model pool empty");
    } else if(functionalityModels.empty())
    {
        throw std::runtime_error("organization_model::OrganizationModelAsk::getFunctionalityMapping: available functionalities empty");
    } else if(functionalSaturationBound.empty())
    {
        std::string msg = "organization_model::OrganizationModelAsk::getFunctionalityMapping: provided empty functionalSaturationBound";
        msg += modelPool.toString() + "\n";
        msg += owlapi::model::IRI::toString(functionalityModels) + "\n";
        msg += functionalSaturationBound.toString() + "\n";
        throw std::runtime_error(msg);
    }

    FunctionalityMapping functionalityMapping(modelPool, functionalityModels, functionalSaturationBound);
    const ModelPool& boundedModelPool = functionalityMapping.getFunctionalSaturationBound();

    numeric::LimitedCombination<owlapi::model::IRI> limitedCombination(boundedModelPool,
            numeric::LimitedCombination<owlapi::model::IRI>::totalNumberOfAtoms(boundedModelPool), numeric::MAX);

    uint32_t count = 0;
    do {
        // Get the current model combination
        IRIList combination = limitedCombination.current();
        // Make sure we have a consistent ordering
        LOG_INFO_S << "Sort";
        {
            base::Time startTime = base::Time::now();
            std::sort(combination.begin(), combination.end());
            base::Time stopTime = base::Time::now();
            LOG_INFO_S << "   | --> required time: " << (stopTime - startTime).toSeconds();
        }

        LOG_INFO_S << "Check combination #" << ++count;
        LOG_INFO_S << "   | --> combination:             " << combination;
        LOG_INFO_S << "   | --> possible functionality models: " << functionalityModels;

        base::Time startTime = base::Time::now();
        // Filter the functionalityModel (from the existing set) which are supported
        // by this very combination
        ModelPool combinationModelPool = OrganizationModel::combination2ModelPool(combination);
        IRIList supportedFunctionalityModels = reasoning::ResourceMatch::filterSupportedModels(combinationModelPool, functionalityModels, mpOrganizationModel->ontology());

        //base::Time stopTime = base::Time::now();
        //LOG_INFO_S << "   | --> required time: " << (stopTime - startTime).toSeconds();
        //LOG_INFO_S << "Update";
        {
            base::Time startTime = base::Time::now();
            // Update the mapping functions - forward and inverse mapping from
            // model/combination to function
            functionalityMapping.add(combinationModelPool, supportedFunctionalityModels);
            base::Time stopTime = base::Time::now();
            LOG_INFO_S << "   | --> required time: " << (stopTime - startTime).toSeconds();
        }
    } while(limitedCombination.next());

    return functionalityMapping;
}

ModelPoolSet OrganizationModelAsk::getResourceSupport(const FunctionalitySet& functionalities) const
{
    if(functionalities.empty())
    {
        std::invalid_argument("organization_model::OrganizationModelAsk::getResourceSupport:"
                " no functionality given in request");
    }

    /// Store the systems that support the functionality
    /// i.e. per requested function the combination of models that support it,
    LOG_DEBUG_S << "FunctionalityMap: " << mFunctionalityMapping.toString();
    Function2PoolMap functionalityProviders;
    {
        FunctionalitySet::const_iterator cit = functionalities.begin();
        for(; cit != functionalities.end(); ++cit)
        {
            const Functionality& functionality = *cit;
            const owlapi::model::IRI& functionalityModel = functionality.getModel();
            try {
                functionalityProviders[functionalityModel] = mFunctionalityMapping.getModelPools(functionalityModel);
            } catch(const std::invalid_argument& e)
            {
                LOG_DEBUG_S << "Could not find resource support for service: '" << functionalityModel;
                return ModelPoolSet();
            }
        }
    }
    LOG_DEBUG_S << "Found functionality providers: " << OrganizationModel::toString(functionalityProviders);

    // Only requested one service
    if(functionalities.size() == 1)
    {
        return functionalityProviders.begin()->second;
    }

    // If looking for a combined system that can provide all the functionalities
    // requested, then the intersection of the sects is the solution of this
    // request
    bool init = true;
    std::set<ModelPool> resultSet;
    std::set<ModelPool> lastResult;
    Function2PoolMap::const_iterator cit = functionalityProviders.begin();
    for(; cit != functionalityProviders.end(); ++cit)
    {
        const ModelPoolSet& currentSet = cit->second;
        if(init)
        {
            resultSet = currentSet;
            init = false;
            continue;
        } else {
            lastResult = resultSet;
        }

        resultSet.clear();
        LOG_DEBUG_S << "Intersection: current set: " << ModelPool::toString(currentSet);
        LOG_DEBUG_S << "Intersection: last set: " << ModelPool::toString(lastResult);
        std::set_intersection(currentSet.begin(), currentSet.end(), lastResult.begin(), lastResult.end(),
                std::inserter(resultSet, resultSet.begin()));
        LOG_DEBUG_S << "Intersection: result" << ModelPool::toString(resultSet);
    }

    return resultSet;
}

ModelPoolSet OrganizationModelAsk::getBoundedResourceSupport(const FunctionalitySet& functionalities) const
{
    ModelPoolSet modelPools = getResourceSupport(functionalities);
    ModelPool bound = getFunctionalSaturationBound(functionalities);
    return applyUpperBound(modelPools, bound);
}

ModelPoolSet OrganizationModelAsk::applyUpperBound(const ModelPoolSet& modelPools, const ModelPool& upperBound) const
{
    ModelPoolSet boundedModelPools;
    ModelPoolSet::const_iterator cit = modelPools.begin();
    for(; cit != modelPools.end(); ++cit)
    {
        const ModelPool& modelPool = *cit;

        LOG_DEBUG_S << "DELTA lva: " << ModelPoolDelta(upperBound).toString();
        LOG_DEBUG_S << "DELTA rval: " << ModelPoolDelta(modelPool).toString();
        ModelPoolDelta delta = Algebra::substract(modelPool, upperBound);
        LOG_DEBUG_S << "RESULT: " << delta.toString();
        if(!delta.isNegative())
        {
            boundedModelPools.insert(modelPool);
        }
    }

    LOG_DEBUG_S << "Upper bound set on resources: " << std::endl
        << "prev: " << ModelPool::toString(modelPools) << std::endl
        << "bound: " << ModelPoolDelta(upperBound).toString() << std::endl
        << "bounded: " << ModelPool::toString(boundedModelPools);
    return boundedModelPools;
}

ModelCombinationSet OrganizationModelAsk::applyUpperBound(const ModelCombinationSet& combinations, const ModelPool& upperBound) const
{
    ModelCombinationSet boundedCombinations;
    ModelCombinationSet::const_iterator cit = combinations.begin();
    for(; cit != combinations.end(); ++cit)
    {
        ModelPool modelPool = OrganizationModel::combination2ModelPool(*cit);

        LOG_DEBUG_S << "DELTA lva: " << ModelPoolDelta(upperBound).toString();
        LOG_DEBUG_S << "DELTA rval: " << ModelPoolDelta(modelPool).toString();
        ModelPoolDelta delta = Algebra::substract(modelPool, upperBound);
        LOG_DEBUG_S << "RESULT: " << delta.toString();
        if(!delta.isNegative())
        {
            boundedCombinations.insert(*cit);
        }
    }

    LOG_DEBUG_S << "Upper bound set on resources: " << std::endl
        << "prev: " << OrganizationModel::toString(combinations) << std::endl
        << "bound: " << ModelPoolDelta(upperBound).toString() << std::endl
        << "bounded: " << OrganizationModel::toString(boundedCombinations);
    return boundedCombinations;
}

ModelPoolSet OrganizationModelAsk::applyLowerBound(const ModelPoolSet& modelPools, const ModelPool& lowerBound) const
{
    ModelPoolSet boundedModelPools;
    ModelPoolSet::const_iterator cit = modelPools.begin();
    for(; cit != modelPools.end(); ++cit)
    {
        const ModelPool& modelPool = *cit;
        ModelPoolDelta delta = Algebra::substract(lowerBound, modelPool);
        if(!delta.isNegative())
        {
            boundedModelPools.insert(modelPool);
        }
    }

    LOG_DEBUG_S << "Lower bound set on resources: " << std::endl
        << "prev: " << ModelPool::toString(modelPools) << std::endl
        << "bound: " << ModelPoolDelta(lowerBound).toString() << std::endl
        << "bounded: " << ModelPool::toString(boundedModelPools);
    return boundedModelPools;
}

ModelCombinationSet OrganizationModelAsk::applyLowerBound(const ModelCombinationSet& combinations, const ModelPool& lowerBound) const
{
    ModelCombinationSet boundedCombinations;
    ModelCombinationSet::const_iterator cit = combinations.begin();
    for(; cit != combinations.end(); ++cit)
    {
        ModelPool modelPool = OrganizationModel::combination2ModelPool(*cit);
        ModelPoolDelta delta = Algebra::substract(lowerBound, modelPool);
        if(!delta.isNegative())
        {
            boundedCombinations.insert(*cit);
        }
    }

    LOG_DEBUG_S << "Lower bound set on resources: " << std::endl
        << "prev: " << OrganizationModel::toString(combinations) << std::endl
        << "bound: " << ModelPoolDelta(lowerBound).toString() << std::endl
        << "bounded: " << OrganizationModel::toString(boundedCombinations);
    return boundedCombinations;
}

ModelPoolSet OrganizationModelAsk::expandToLowerBound(const ModelPoolSet& modelPools, const ModelPool& lowerBound) const
{
    ModelPoolSet boundedModelPools;
    if(modelPools.empty())
    {
        boundedModelPools.insert(lowerBound);
        return boundedModelPools;
    }

    ModelPoolSet::const_iterator cit = modelPools.begin();
    for(; cit != modelPools.end(); ++cit)
    {
        ModelPool modelPool = *cit;
        // enforce minimum requirement
        modelPool = Algebra::max(lowerBound, modelPool);
        boundedModelPools.insert(modelPool);
    }

    LOG_DEBUG_S << "Lower bound expanded on resources: " << std::endl
        << "prev: " << ModelPool::toString(modelPools) << std::endl
        << "bound: " << ModelPoolDelta(lowerBound).toString() << std::endl
        << "bounded: " << ModelPool::toString(boundedModelPools);
    return boundedModelPools;
}

ModelCombinationSet OrganizationModelAsk::expandToLowerBound(const ModelCombinationSet& combinations, const ModelPool& lowerBound) const
{
    ModelCombinationSet boundedCombinations;
    if(combinations.empty())
    {
        boundedCombinations.insert(OrganizationModel::modelPool2Combination(lowerBound));
    }
    ModelCombinationSet::const_iterator cit = combinations.begin();
    for(; cit != combinations.end(); ++cit)
    {
        ModelPool modelPool = OrganizationModel::combination2ModelPool(*cit);
        // enforce minimum requirement
        modelPool = Algebra::max(lowerBound, modelPool);
        ModelCombination expandedCombination = OrganizationModel::modelPool2Combination(modelPool);

        boundedCombinations.insert(expandedCombination);
    }

    LOG_DEBUG_S << "Lower bound expanded on resources: " << std::endl
        << "prev: " << OrganizationModel::toString(combinations) << std::endl
        << "bound: " << ModelPoolDelta(lowerBound).toString() << std::endl
        << "bounded: " << OrganizationModel::toString(boundedCombinations);
    return boundedCombinations;
}

algebra::SupportType OrganizationModelAsk::getSupportType(const Functionality& functionality, const owlapi::model::IRI& model, uint32_t cardinalityOfModel) const
{
    algebra::ResourceSupportVector functionalitySupportVector = getSupportVector(functionality.getModel(), IRIList(), false /*useMaxCardinality*/);
    algebra::ResourceSupportVector modelSupportVector =
        getSupportVector(model, functionalitySupportVector.getLabels(), true /*useMaxCardinality*/)*
        static_cast<double>(cardinalityOfModel);

    return functionalitySupportVector.getSupportFrom(modelSupportVector, *this);
}

uint32_t OrganizationModelAsk::getFunctionalSaturationBound(const owlapi::model::IRI& requirementModel, const owlapi::model::IRI& model) const
{
    LOG_DEBUG_S << "Get functional saturation bound for " << requirementModel << " for model '" << model << "'";
    // Collect requirements, i.e., max cardinalities
    algebra::ResourceSupportVector requirementSupportVector = getSupportVector(requirementModel, IRIList(), false /*useMaxCardinality*/);
    if(requirementSupportVector.isNull())
    {
        owlapi::model::IRIList labels;
        labels.push_back(requirementModel);
        base::VectorXd required(1);
        required(0) = 1;
        requirementSupportVector = algebra::ResourceSupportVector(required, labels);
        LOG_DEBUG_S << "functionality support vector is null : using " << requirementSupportVector.toString();
    } else {
        LOG_DEBUG_S << "Get model support vector";
    }
    // Collect available resources -- and limit to the required ones
    // (getSupportVector will accumulate all (subclass) models)
    algebra::ResourceSupportVector modelSupportVector = getSupportVector(model, requirementSupportVector.getLabels(), true /*useMaxCardinality*/);
    LOG_DEBUG_S << "Retrieved model support vector";

    // Expand the support vectors to account for subclasses within the required
    // scope
    requirementSupportVector = requirementSupportVector.embedClassRelationship(*this);
    modelSupportVector = modelSupportVector.embedClassRelationship(*this);

    // Compute the support ratios
    algebra::ResourceSupportVector ratios = requirementSupportVector.getRatios(modelSupportVector);

    LOG_DEBUG_S << "Requirement: " << requirementSupportVector.toString();
    LOG_DEBUG_S << "Provider: " << modelSupportVector.toString();
    LOG_DEBUG_S << "Ratios: " << ratios.toString();

    // max in the set of ratio tells us how many model instances
    // contribute to fulfill this service (even partially)
    double max = 0.0;
    for(uint32_t i = 0; i < ratios.size(); ++i)
    {
        double val = ratios(i);
        if(val != std::numeric_limits<double>::quiet_NaN())
        {
            if(val > max)
            {
                max = val;
            }
        }
    }
    return static_cast<uint32_t>( std::ceil(max) );
}

ModelPool OrganizationModelAsk::getFunctionalSaturationBound(const Functionality& functionality) const
{
    if(mModelPool.empty())
    {
        throw std::invalid_argument("organization_model::OrganizationModelAsk::getFunctionalSaturationBound:"
                " model pool is empty. Call OrganizationModelAsk::prepare with model pool");
    }

    ModelPool upperBounds;
    ModelPool::const_iterator cit = mModelPool.begin();
    for(; cit != mModelPool.end(); ++cit)
    {
        uint32_t saturation = getFunctionalSaturationBound(functionality.getModel(), cit->first);
        upperBounds[cit->first] = saturation;
    }
    return upperBounds;
}

ModelPool OrganizationModelAsk::getFunctionalSaturationBound(const FunctionalitySet& functionalities) const
{
    ModelPool upperBounds;
    FunctionalitySet::const_iterator cit = functionalities.begin();
    for(; cit != functionalities.end(); ++cit)
    {
        ModelPool saturation = getFunctionalSaturationBound(*cit);

        ModelPool::const_iterator mit = saturation.begin();
        for(; mit != saturation.end(); ++mit)
        {
            upperBounds[mit->first] = std::max(upperBounds[mit->first], saturation[mit->first]);
        }
    }
    return upperBounds;
}

bool OrganizationModelAsk::canBeDistinct(const ModelCombination& a, const ModelCombination& b) const
{
    ModelPool poolA = OrganizationModel::combination2ModelPool(a);
    ModelPool poolB = OrganizationModel::combination2ModelPool(b);

    ModelPoolDelta totalRequirements = Algebra::sum(poolA, poolB);
    ModelPoolDelta delta = Algebra::delta(totalRequirements, mModelPool);

    return delta.isNegative();
}

bool OrganizationModelAsk::isSupporting(const ModelPool& modelPool, const FunctionalitySet& functionalities) const
{
    FunctionalitySet::const_iterator cit = functionalities.begin();
    ModelPoolSet previousModelPools;
    bool init = true;
    for(; cit != functionalities.end(); ++cit)
    {
        const Functionality& functionality = *cit;
        try {
            const ModelPoolSet& modelPools = mFunctionalityMapping.getModelPools(functionality.getModel());

            if(init)
            {
                previousModelPools = modelPools;
                init = false;
                continue;
            }

            ModelPoolSet resultList;
            std::set_intersection(modelPools.begin(), modelPools.end(),
                    previousModelPools.begin(), previousModelPools.end(),
                    std::inserter(resultList, resultList.begin()) );
             previousModelPools.insert(resultList.begin(), resultList.end());

        } catch(const std::invalid_argument& e)
        {
            throw std::runtime_error("organization_model::OrganizationModelAsk::isSupporting \
                    could not find functionality '" + functionality.getModel().toString() + "'");
        }

    }

    ModelPoolSet::const_iterator pit = std::find(previousModelPools.begin(), previousModelPools.end(), modelPool);
    if(pit != previousModelPools.end())
    {
        return true;
    } else {
        return false;
    }
}

bool OrganizationModelAsk::isSupporting(const owlapi::model::IRI& model, const Functionality& functionality) const
{
    ModelPool modelPool;
    modelPool.setResourceCount(model, 1);

    FunctionalitySet functionalities;
    functionalities.insert(functionality);
    if( isSupporting(modelPool, functionalities) )
    {
        LOG_DEBUG_S << "model '" << model << "' supports '" << functionality.getModel() << "'";
        return true;
    } else {
        LOG_DEBUG_S << "model '" << model << "' does not support '" << functionality.getModel() << "'";
        return false;
    }  
}

algebra::ResourceSupportVector OrganizationModelAsk::getSupportVector(const owlapi::model::IRI& model,
        const owlapi::model::IRIList& filterLabels,
        bool useMaxCardinality) const
{
    using namespace owlapi::model;
    std::vector<OWLCardinalityRestriction::Ptr> restrictions = mOntologyAsk.getCardinalityRestrictions(model);
    if(restrictions.empty())
    {
        return algebra::ResourceSupportVector();
    } else {
        std::map<IRI, OWLCardinalityRestriction::MinMax> modelCount = OWLCardinalityRestriction::getBounds(restrictions);
        LOG_WARN_S << "ModelCount: "<< modelCount.size() << ", restrictions: " << restrictions.size();
        if(restrictions.size() == 1)
        {
            LOG_WARN_S << restrictions[0]->toString();
        }
        return getSupportVector(modelCount, filterLabels, useMaxCardinality);
    }

}

algebra::ResourceSupportVector OrganizationModelAsk::getSupportVector(const std::map<owlapi::model::IRI,
    owlapi::model::OWLCardinalityRestriction::MinMax>& modelBounds,
    const owlapi::model::IRIList& filterLabels, bool useMaxCardinality) const
{
    if(modelBounds.size() == 0)
    {
        throw std::invalid_argument("organization_model::OrganizationModelAsk::getSupportVector: no model bounds given");
    }

    using namespace owlapi::model;

    base::VectorXd vector;
    std::vector<IRI> labels;

    if(filterLabels.empty())
    {
        // Create zero vector: each row maps to a model
        vector = base::VectorXd::Zero(modelBounds.size());

        std::map<IRI, OWLCardinalityRestriction::MinMax>::const_iterator cit = modelBounds.begin();
        uint32_t i = 0;
        for(; cit != modelBounds.end(); ++cit)
        {
            labels.push_back(cit->first);
            // using max value
            if(useMaxCardinality)
            {
                vector(i++) = cit->second.second;
            } else {
                vector(i++) = cit->second.first;
            }
        }
    } else {
        labels = filterLabels;
    }

    LOG_DEBUG_S << "Use labels: " << labels;
    vector = base::VectorXd::Zero(labels.size());

    std::vector<IRI>::const_iterator cit = labels.begin();
    uint32_t dimension = 0;
    for(; cit != labels.end(); ++cit)
    {
        const IRI& dimensionLabel = *cit;
        std::map<IRI, OWLCardinalityRestriction::MinMax>::const_iterator mit = modelBounds.begin();
        for(; mit != modelBounds.end(); ++mit)
        {
            const IRI& modelDimensionLabel = mit->first;
            LOG_DEBUG_S << "Check model support for " << dimensionLabel << "  from " << modelDimensionLabel;

            // Sum the requirement/availability of this model type
            if(dimensionLabel == modelDimensionLabel || mOntologyAsk.isSubClassOf(modelDimensionLabel, dimensionLabel))
            {
                if(useMaxCardinality)
                {
                    LOG_DEBUG_S << "update " << dimension << " with " << mit->second.second << " -- min is "<< mit->second.first;
                    // using max value
                    vector(dimension) += mit->second.second;
                } else {
                    // using min value
                    LOG_DEBUG_S << "update " << dimension << " with min " << mit->second.first << " -- max is "<< mit->second.second;
                    vector(dimension) += mit->second.first;
                }
            } else {
                LOG_DEBUG_S << "No support";
                // nothing to do
            }
        }
        ++dimension;
    }

    LOG_DEBUG_S << "Get support vector" << std::endl
        << "    " << vector << std::endl
        << "    " << labels;

    return algebra::ResourceSupportVector(vector, labels);
}

std::string OrganizationModelAsk::toString() const
{
    std::stringstream ss;
    ss << "FunctionalityMapping:" << std::endl;
    ss << mFunctionalityMapping.toString() << std::endl;

    ModelPoolDelta mp(mModelPool);
    ss << mp.toString() << std::endl;
    return ss.str();
}

//owlapi::model::IRIList OrganizationModelAsk::filterSupportedModels(const owlapi::model::IRIList& combinations,
//        const owlapi::model::IRIList& serviceModels)
//{
//    using namespace owlapi::model;
//
//    std::vector<OWLCardinalityRestriction::Ptr> providerRestrictions =
//            ontology()->getCardinalityRestrictions(combinations);
//    owlapi::model::IRIList supportedModels;
//
//    owlapi::model::IRIList::const_iterator it = serviceModels.begin();
//    for(; it != serviceModels.end(); ++it)
//    {
//        owlapi::model::IRI serviceModel = *it;
//        std::vector<OWLCardinalityRestriction::Ptr> serviceRestrictions =
//            ontology()->getCardinalityRestrictions(serviceModel);
//
//        std::map<IRI, OWLCardinalityRestriction::MinMax> serviceModelCount = OWLCardinalityRestriction::getBounds(serviceRestrictions);
//        // get minimum requirements
//        bool useMaxCardinality = false;
//        ResourceSupportVector serviceSupportVector = getSupportVector(serviceModelCount, IRIList(), useMaxCardinality);
//
//        std::map<IRI, OWLCardinalityRestriction::MinMax> providerModelCount = OWLCardinalityRestriction::getBounds(providerRestrictions);
//        // Use all available
//        useMaxCardinality = true;
//        ResourceSupportVector providerSupportVector = getSupportVector(providerModelCount, serviceSupportVector.getLabels(), useMaxCardinality);
//
//        if( serviceSupportVector.fullSupportFrom(providerSupportVector) )
//        {
//            supportedModels.push_back(serviceModel);
//            LOG_DEBUG_S << "Full support for: ";
//            LOG_DEBUG_S << "    service model: " << serviceModel;
//            LOG_DEBUG_S << "    from combination of provider models: " << combinations;
//        }
//    }
//    return supportedModels;
//}

} // end namespace organization_model
