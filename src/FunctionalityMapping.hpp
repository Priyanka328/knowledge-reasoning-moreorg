#ifndef ORGANIZATION_MODEL_FUNCTIONALITY_MAPPING_HPP
#define ORGANIZATION_MODEL_FUNCTIONALITY_MAPPING_HPP

#include <set>
#include<organization_model/ModelPool.hpp>

namespace organization_model {

/// Maps a 'combined system' to the functionality it can 'theoretically'
/// provide when looking at its resources
typedef std::map<ModelPool, owlapi::model::IRIList> Pool2FunctionMap;
typedef std::map<owlapi::model::IRI, ModelPool::Set > Function2PoolMap;

/**
 * \class FunctionalityMapping
 * \brief The FunctionalityMapping class allows to cache the mapping between
 * models and their respective functionalities
 * \details Models or combinations are represented as ModelPools and
 * functionality can be provided by sets of such ModelPools (eventually
 * representing combination of systems)
 */
class FunctionalityMapping
{
    /// The resources that are available
    ModelPool mModelPool;
    /// The list of known functionalities
    owlapi::model::IRIList mFunctionalities;
    owlapi::model::IRISet mSupportedFunctionalities;

    /// The global functional saturation bound (for all known/considered
    //functionalities))
    ModelPool mFunctionalSaturationBound;

    /// Cache to map from a function to supported ModelPools
    Function2PoolMap mFunction2Pool;

public:
    FunctionalityMapping();

    /**
     * Create a functionality mapping
     *
     * \param modelPool the available resources
     * \param functionalities List of model to compute the mapping for
     * \param functionalSaturationBound The functionalSaturationBound that
     * should be taken into account
     */
    FunctionalityMapping(const ModelPool& modelPool,
        const owlapi::model::IRIList& functionalities,
        const ModelPool& functionalSaturationBound);

    /**
     * Get the list of ModelPools that support a given function
     * \param functionModel IRI of the function model
     * \return set of ModelPool that support the function
     */
    const ModelPool::Set& getModelPools(const owlapi::model::IRI& functionModel) const;

    /**
     * Set the model pool
     */
    void setModelPool(const ModelPool& modelPool) { mModelPool = modelPool; }

    /**
     * Get the model pool
     */
    const ModelPool& getModelPool() const { return mModelPool; }

    /**
     * Set the general functional saturation bound
     */
    void setFunctionalSaturationBound(const ModelPool& bounds) { mFunctionalSaturationBound = bounds; }

    /**
     * Retrieve the general functional saturation bound
     */
    const ModelPool& getFunctionalSaturationBound() const { return mFunctionalSaturationBound; }

    /**
     * Add a supported function for a model pool
     * \param modelPool ModelPool that support the function
     * \param function Model of the supported function
     */
    void add(const ModelPool& modelPool, const owlapi::model::IRI& functionModel);

    /**
     * Add a list of supported function models for a model pool
     * \param modelPool ModelPool that support the function
     * \param functionModels Models of the supported functions
     */
    void add(const ModelPool& modelPool, const owlapi::model::IRIList& functionModels);

    /**
     * Stringify object
     * \param indent Indentation in number of spaces
     * \return string representation of FunctionalityMapping
     */
    std::string toString(uint32_t indent = 0) const;


    /**
     * Compute the list of supported functionalities, i.e.
     * when at least some combination of models supports this functionality
     * \return list of supported functionalities
     */
    owlapi::model::IRISet getSupportedFunctionalities() const { return mSupportedFunctionalities; }
};

} // end namespace organization_model
#endif // ORGANIZATION_MODEL_FUNCTIONALITY_MAPPING_HPP
