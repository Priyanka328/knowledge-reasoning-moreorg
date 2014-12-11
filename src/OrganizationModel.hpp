#ifndef OWL_OM_ORGANIZATION_MODEL_HPP
#define OWL_OM_ORGANIZATION_MODEL_HPP

#include <stdint.h>
#include <owl_om/owlapi/model/OWLOntology.hpp>
#include <owl_om/organization_model/ActorModelLink.hpp>
#include <owl_om/organization_model/InterfaceConnection.hpp>
#include <owl_om/organization_model/Grounding.hpp>
#include <owl_om/organization_model/Statistics.hpp>

namespace owl = owlapi::model;

namespace owl_om {

typedef std::vector<owl::IRIList> CandidatesList;

/**
 * \mainpage Organization modelling with OWL
 */
class OrganizationModel
{
public:

    typedef boost::shared_ptr<OrganizationModel> Ptr;
    typedef std::map< owl::IRI, owl::IRIList> IRI2IRIListCache;
    typedef std::map< owl::IRI, owl::IRI> IRI2IRICache;
    typedef std::map< std::pair<owl::IRI,owl::IRI>, owl::IRIList> RelationCache;
    typedef std::map< std::pair<owl::IRI,owl::IRI>, bool> RelationPredicateCache;
    typedef std::map< owl::IRI, owl::IRISet> IRI2IRISetCache;
    typedef std::map< owl::IRI, std::vector<owl::OWLCardinalityRestriction::Ptr> > IRI2RestrictionsCache;

    /**
     * Constructor to create an OrganizationModel from an existing description file
     * \param filename File name to an rdf/xml formatted ontology description file
     */
    OrganizationModel(const std::string& filename = "");

    /**
     * Update the organization model
     */
    void refresh(bool performInference = true);

    owlapi::model::OWLOntology::Ptr ontology() { return mpOntology; }

    const owlapi::model::OWLOntology::Ptr ontology() const { return mpOntology; }

    /**
     * Create and add a instance of a given elementary klass
     */
    void createInstance(const owl::IRI& instanceName, const owl::IRI& klass);

    /**
     * Try to map requirements to provider
     * \param resourceRequirement Resource requirement
     * \param availableResources List of available resources
     * \param resourceProvider Optional label for the resource provider
     * \param requirementModel Optional label for requirement model that the requirements originate from
     * \return corresponding grounding, which need to be check on completness
     */
    organization_model::Grounding resolveRequirements(const std::vector<owlapi::model::OWLCardinalityRestriction::Ptr>& resourceRequirements, const owl::IRIList& availableResources, const owl::IRI& resourceProvider = owl::IRI(), const owl::IRI& requirementModel = owl::IRI()) const;

    // PREDICATES
    /**
     * Test if a set of interfaces is compatible (in general)
     * Test the relation 'compatibleWith'
     *
     */
    bool checkIfCompatible(const owl::IRI& instance, const owl::IRI& otherInstance);

    /**
     * Run inference to identify services and capabilites that are provided
     * by actors
     */
    void runInferenceEngine();

    /**
     * Reduce list of actor to unique individuals, i.e. removing
     * alias and same individuals
     */
    owl::IRIList compactActorList();

    /**
     * Get resource model of this instance (type of this instance)
     * \return IRI of resource model
     */
    owl::IRI getResourceModel(const owl::IRI& instance) const;

    /**
     * Check if first model fullfills second (subsumption)
     * \return True upon success, false otherwise
     */
    bool fulfills(const owl::IRI& model, const owl::IRI& otherModel) const;

    /**
     * Create new instance (ABox from existing model (model is defined in TBox,
     * and possibly detailed in ABox via punning)
     * Model will be associated via modelledBy relation and subclassed from
     * classType, the name for this instance will be autogenerated
     * \param classType New instance will be of the given classType
     * \param createRequiredResources Set to true if required objects should be created, i.e. autofufill the cardinality restrictions
     * \see http://www.w3.org/TR/owl2-new-features/#Simple_metamodeling_capabilities
     * \return IRI of the instance, the name for this instance will be automatically
     * generate based on the number of existing instances
     * (<classtype>_<globalcount>)
     */
    owl::IRI createNewInstance(const owl::IRI& classType, bool createRequiredResources = false) const;

    /**
     * Generate a combination list based on actor link combination, i.e. here the 
     * link list for combined actor is constructed
     *
     * \return List of interface combinations, where each interface combination represents
     * a combined actor
     */
    organization_model::InterfaceCombinationList generateInterfaceCombinations();

    organization_model::InterfaceCombinationList generateInterfaceCombinationsCCF();

    /**
     * Compute upper bound for actor combinations
     * \return upper bound
     */
    uint32_t upperCombinationBound();

    /**
     * Statistics
     */
    std::vector<organization_model::Statistics> getStatistics() { return mStatistics; }

    organization_model::Statistics getCurrentStatistics() { return mCurrentStats; }

    void setMaximumNumberOfLinks(uint32_t n) { mMaximumNumberOfLinks = n; }
    uint32_t getMaximumNumberOfLinks() { return mMaximumNumberOfLinks; }

    owl::IRI getRelatedProviderInstance(const owl::IRI& actor, const owl::IRI& model);

    void setDouble(const owl::IRI& iri, const owl::IRI& dataProperty, double val);


private:

    /**
     * Add a provides relationship to for the given actor and a model
     */
    void addProvider(const owl::IRI& actor, const owl::IRI& model);

    /**
     * Check if model is provider for a given model, i.e.
     * remaps requirements of actorModel as availableResources
     * and checks whether this is sufficient for the given model
     *
     * Make sure service and capabilies that depend on other are check AFTER
     * their dependencies -- since this model provider creates cache from 
     * this values
     * \return True upon success, false otherwise
     */
    bool isModelProvider(const owl::IRI& actorModel, const owl::IRI& model) const;

    /**
     * Cached version
     *
     * Checks if the associated actor model is providing the given model
     */
    bool isProviding(const owl::IRI& actor, const owl::IRI& model) const;

    /**
     * Cached version
     */
    owl::IRIList allRelatedInstances(const owl::IRI& actor, const owl::IRI& relation) const;


    /**
     * Retrieve the model requirements, based on the associated cardinality
     * restrictions
     * \return List of cardinality restrictions for the given model
     */
    std::vector<owlapi::model::OWLCardinalityRestriction::Ptr> getModelRequirements(const owl::IRI& model) const;

    /**
     * Sort the list so that items with dependencies
     * are listed after their dependencies
     */
    owl::IRIList sortByDependency(const owl::IRIList& list);

    /**
     * Check if item has a dependency on other model
     * i.e. main -> Service
     * other -> Service
     */
    bool hasModelDependency(const owl::IRI& main, const owl::IRI& other);

    /**
     * Create a new coalition model name from a given set of requirements (i.e.
     * actor models)
     */
    owl::IRI createCoalitionModelName(const std::vector<owlapi::model::OWLCardinalityRestriction::Ptr>& actorModelRequirements);

    /**
     * Creates a new coalition model based on a given 
     * Registers a new class of actor if necessary for this actor
     * \return IRI of new actor, or empty IRI if the actor already exists
     */
    owl::IRI createNewCoalitionModel(const std::vector<owlapi::model::OWLCardinalityRestriction::Ptr>& actorModelRequirement);

    /**
     * Infer new instance of a given class type, base on checking whether a given model
     * is fulfilled
     *
     * Will associate the given actor with each model
     * \param actor Actor to perform inference for, actor will in effect have two types of relationships:
     *     (a) provides model
     *     (b) has instance of the class type  (where instance is modelledBy the given model)
     * \param models List of models that should be check
     */
    owl::IRIList infer(const owl::IRI& actor, const owl::IRIList& models);

    /**
     * Compute a list of resources that should be available if the restrictions
     * should be fulfilled
     *
     * MIN/MAX are translated into exact requirements
     */
    owl::IRIList exactRequiredResources(const std::vector<owlapi::model::OWLCardinalityRestriction::Ptr>& restrictions) const;

    owlapi::model::OWLOntology::Ptr mpOntology;
    owlapi::model::OWLOntologyTell::Ptr mpTell;
    owlapi::model::OWLOntologyAsk::Ptr mpAsk;

    organization_model::Statistics mCurrentStats;
    std::vector<organization_model::Statistics> mStatistics;

    mutable IRI2RestrictionsCache mModelRequirementsCache;
    mutable RelationCache mRelationsCache;
    mutable IRI2IRICache mResourceModelCache;
    mutable RelationPredicateCache mProviderCache;
    mutable RelationPredicateCache mCompatibilityCache;

    // Tell if model is a provider of a given service
    mutable RelationPredicateCache mModelProviderCache;
    // Get list of provided service per model
    mutable IRI2IRISetCache mModelProviderSetCache;

    owl::IRIList mServices;
    owl::IRIList mCapabilities;

    uint32_t mCompositeActorModelsCount;
    uint32_t mCompositeActorsCount;

    uint32_t mMaximumNumberOfLinks;

    std::vector< std::vector<organization_model::ActorModelLink> > mCompositeActorModels;
};

} // end namespace owl_om
#endif // OWL_OM_ORGANIZATION_MODEL_HPP
