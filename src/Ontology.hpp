#ifndef OWL_OM_DB_ONTOLOGY_HPP
#define OWL_OM_DB_ONTOLOGY_HPP

#include <string>
#include <boost/shared_ptr.hpp>
#include <owl_om/KnowledgeBase.hpp>
#include <owl_om/db/rdf/SparqlInterface.hpp>
#include <owl_om/OWLApi.hpp>

//! Main enclosing namespace for OWL Organization Modelling
namespace owl_om {

class OrganizationModel;

/**
 * @class KnowledgeBase
 * @brief KnowledgeBase serves as main layer of abstraction for the underlying database
 *
 * @see http://www.w3.org/TR/owl-parsing/#sec-implementation
 */
class Ontology : public owl_om::KnowledgeBase
{
    friend class OrganizationModel;

    // Mapping of iri to types
    std::map<owlapi::model::IRI, owlapi::model::OWLClass::Ptr> mClasses;
    std::map<owlapi::model::IRI, owlapi::model::OWLNamedIndividual::Ptr> mNamedIndividuals;
    std::map<owlapi::model::IRI, owlapi::model::OWLAnonymousIndividual::Ptr> mAnonymousIndividuals;
    std::map<owlapi::model::IRI, owlapi::model::OWLObjectProperty::Ptr> mObjectProperties;
    std::map<owlapi::model::IRI, owlapi::model::OWLDataProperty::Ptr> mDataProperties;

    /// General axiom map
    std::map<owlapi::model::OWLAxiom::AxiomType, std::vector<owlapi::model::OWLAxiom::Ptr> > mAxiomsByType;

    std::map<owlapi::model::OWLClassExpression::Ptr, std::vector<owlapi::model::OWLClassAssertionAxiom::Ptr> > mClassAssertionAxiomsByClass;

    std::map<owlapi::model::OWLIndividual::Ptr, std::vector<owlapi::model::OWLClassAssertionAxiom::Ptr> > mClassAssertionAxiomsByIndividual;

    std::map<owlapi::model::OWLDataProperty::Ptr, std::vector<owlapi::model::OWLAxiom::Ptr> > mDataPropertyAxioms;
    std::map<owlapi::model::OWLObjectProperty::Ptr, std::vector<owlapi::model::OWLAxiom::Ptr> > mObjectPropertyAxioms;
    std::map<owlapi::model::OWLNamedIndividual::Ptr, std::vector<owlapi::model::OWLAxiom::Ptr> > mNamedIndividualAxioms;
    /// Map of anonymous individual to all axioms the individual is involved into
    std::map<owlapi::model::OWLAnonymousIndividual::Ptr, std::vector<owlapi::model::OWLAxiom::Ptr> > mAnonymousIndividualAxioms;
    std::map<owlapi::model::OWLEntity::Ptr, std::vector<owlapi::model::OWLDeclarationAxiom::Ptr> > mDeclarationsByEntity;


    /// Map to access subclass axiom by a given subclass
    std::map<owlapi::model::OWLClassExpression::Ptr, std::vector<owlapi::model::OWLSubClassOfAxiom::Ptr> > mSubClassAxiomBySubPosition;
    /// Map to access subclass axiom by a given superclass
    std::map<owlapi::model::OWLClassExpression::Ptr, std::vector<owlapi::model::OWLSubClassOfAxiom::Ptr> > mSubClassAxiomBySuperPosition;

public:
    /**
     * Contructs an empty ontology
     * Prefer using Ontology::fromFile
     */
    Ontology();

    /**
     * Deconstructor implementation
     */
    ~Ontology();

    typedef boost::shared_ptr<Ontology> Ptr;

    /**
     * Create knowledge base from file
     */
    static Ontology::Ptr fromFile(const std::string& filename);

    /**
     * Update the ontology
     */
    void reload();

    /**
     * Find all items that match the query
     */
    db::query::Results findAll(const db::query::Variable& subject, const db::query::Variable& predicate, const db::query::Variable& object) const;

    /**
     * Convert ontology to string representation
     * \return Stringified ontology -- mainly for debugging, do not use for
     * serialization
     */
    std::string toString() const;

    /**
     * Compute restrictions on a given class
     * \return List of restrictions
     */
    std::vector<owlapi::model::OWLCardinalityRestriction::Ptr> getCardinalityRestrictions(const owlapi::model::IRI& klass);

    /**
     * Compute restrictions on classes
     * \return Map where class IRIs are mapped to restrictions that reply to each class
     */
    std::map<owlapi::model::IRI, std::vector<owlapi::model::OWLCardinalityRestriction::Ptr> > getCardinalityRestrictions(const std::vector<owlapi::model::IRI>& klasses);

    /**
     * Get or create the OWLClass instance by IRI
     * \return OWLClass::Ptr
     */
    owlapi::model::OWLClass::Ptr getOWLClass(const IRI& iri);

    /**
     * Get or create the OWLAnonymousIndividual by IRI
     * \return OWLAnonymousIndividual::Ptr
     */
    owlapi::model::OWLAnonymousIndividual::Ptr getOWLAnonymousIndividual(const IRI& iri);

    /**
     * Get or create the OWLNamedIndividual by IRI
     * \return OWLNamedIndividual::Ptr
     */
    owlapi::model::OWLNamedIndividual::Ptr getOWLNamedIndividual(const IRI& iri);

    /**
     * Get or create the OWLObjectProperty by IRI
     * \return OWLObjectProperty::Ptr
     */
    owlapi::model::OWLObjectProperty::Ptr getOWLObjectProperty(const IRI& iri);

    /**
     * Get or create the OWLDataProperty by IRI
     * \return OWLDataProperty::Ptr
     */
    owlapi::model::OWLDataProperty::Ptr getOWLDataProperty(const IRI& iri);
private:

    /**
     * Initialize all default classes used in the ontology
     */
    void initializeDefaultClasses();

    /**
     * Load all object properties
     */
    void loadObjectProperties();

    /**
     * Add an axiom
     * \param axiom
     */
    void addAxiom(owlapi::model::OWLAxiom::Ptr axiom);

    /// Pointer to the underlying query interfaces for SPARQL
    db::query::SparqlInterface* mSparqlInterface;
};

} // end namespace owl_om

#endif // OWL_OM_DB_ONTOLOGY_HPP
