#ifndef SAS_PLUS_TRANSITION_H
#define SAS_PLUS_TRANSITION_H

#include <set>
#include <map>
#include <vector>
#include <iostream>

#include "../plan_types.h"
#include "../pointers.h"
#include "dtg_types.h"
#include "plan.h"

namespace MyPOP {

class Predicate;
class Atom;
class Action;
class Bindings;
class Object;
class Term;
class Variable;
	
namespace SAS_Plus {

class DomainTransitionGraphManager;
class DomainTransitionGraphNode;
class DomainTransitionGraph;
class Transition;
class BoundedAtom;
class PropertySpace;

/**
 * To make my life easier I created a couple of function to help executing functions like std::remove_if.
 */
namespace Utilities {

	/**
	 * Check if the given DTG node is the destination node for the given transition.
	 */
	struct TransitionToNodeEquals : public std::binary_function<const Transition*, const DomainTransitionGraphNode*, bool>
	{
		bool operator()(const Transition* transition, const DomainTransitionGraphNode* dtg_node) const;
	};
};

class BalancedPropertySet;

struct CompareBalancedPropertySet {
	bool operator()(const BalancedPropertySet& lhs, const BalancedPropertySet& rhs) const;
};

class BalancedPropertySet {
	
public:
	
	BalancedPropertySet(const PropertySpace& property_space);
	
	void removeProperty(const BoundedAtom& fact);
	
	void addProperty(const BoundedAtom& fact);
	
	const std::vector<const BoundedAtom*>& getAddedProperties() const;
	
	const std::vector<const BoundedAtom*>& getRemovedProperties() const;
	
	void removeAddedProperty(const BoundedAtom& fact);
	
	void removeRemovedProperty(const BoundedAtom& fact);
	
private:
	const PropertySpace* property_space_;
	std::vector<const BoundedAtom*> properties_added_;
	std::vector<const BoundedAtom*> properties_deleted_;
	
	friend bool CompareBalancedPropertySet::operator()(const BalancedPropertySet& lhs, const BalancedPropertySet& rhs) const;
};

/**
 * The transition class marks a transition between two nodes in a DTG.
 */
class Transition
{
public:
	
	/**
	 * Try to create a new transition from the from_node to the to_node. The following assumptions need to be satisfied for this function to work:
	 * 1) There is one and only one shared property being exchanged. This holds for all transitions rules generated by TIM.
	 * 2) No existing bindings exist between the given nodes.
 	 * @param action The action to apply to the from node.
	 * @param from_node The start node of the transition.
	 * @param to_node The end node of the transition.
	 * @return The generated transition or NULL if the action cannot be applied.
	 */
	static Transition* createTransition(const Action& action, DomainTransitionGraphNode& from_node, DomainTransitionGraphNode& to_node);
	
	/**
	 * Try to create a new transition which satisfies the single fact in to_node (I know!).
 	 * @param action The action to apply to the from node.
	 * @param from_node The start node of the transition which is empty.
	 * @param to_node The end node of the transition which contains a single fact which can be reached by action.
	 * @return The generated transition or NULL if the action cannot be applied.
	 */
	static Transition* createSimpleTransition(const StepPtr action_step, DomainTransitionGraphNode& from_node, DomainTransitionGraphNode& to_node);
	
	~Transition();
	
	/**
	 * Migrate a transition from a pair of nodes to their clones.
	 */
	Transition* migrateTransition(DomainTransitionGraphNode& from_node, DomainTransitionGraphNode& to_node) const;
	
	Transition* migrateTransition(MyPOP::SAS_Plus::DomainTransitionGraphNode& from_node, MyPOP::SAS_Plus::DomainTransitionGraphNode& to_node, int from_fact_ordering[], int to_fact_ordering[]) const;
	
	/**
	 * As we update the from and to nodes of the transition with new nodes we need to update the lists of preconditions linking to the from
	 * node. Also we will automatically update the persistent facts as well.
	 * @param precondition The precondition which has just been added to the from node of this transition.
	 */
	void addedFromNodePrecondition(const Atom& precondition);
	
	/**
	 * As we update the from and to nodes of the transition with new nodes we need to update the lists of preconditions and effects linking to
	 * the to node. Also we will automatically update the persistent facts as well.
	 * @param fact The precondition or effect which has just been added to the to node of this transition - we automatically detect which.
	 */
	void addedToNodeFact(const Atom& fact);
	
	/**
	 * Get the Step ID how the action is bound.
	 */
	inline StepID getStepId() const { return step_->getStepId(); }
	
	/**
	 * Get the Action.
	 */
	inline const Action& getAction() const { return step_->getAction(); }

	/**
	 * Get the start node.
	 */
	inline DomainTransitionGraphNode& getFromNode() const { return *from_node_; }

	/**
	 * Get the end node.
	 */
	inline DomainTransitionGraphNode& getToNode() const { return *to_node_; }
	
	/**
	 * Get all the preconditions of the transition in the order as defined in the PDDL file.
	 */
	inline const std::vector<std::pair<const Atom*, InvariableIndex> >& getAllPreconditions() const { return *all_preconditions_; }
	
	/**
	 * Get all the effects of the transition in the order as defined in the PDDL file.
	 */
	inline const std::vector<std::pair<const Atom*, InvariableIndex> >& getAllEffects() const { return *all_effects_; }
	
	/**
	 * Get all the preconditions which are linked to the from node. They are indexed according to the way facts are ordered in the from Node.
	 * If a fact is not linked to a precondition then the value will be equal to <NULL, NO_INVARIABLE_INDEX>.
	 */
	inline const std::vector<std::pair<const Atom*, InvariableIndex> >& getFromNodePreconditions() const { return *from_node_preconditions_; }
	
	/**
	 * Get all the effects which are linked to the to node. They are indexed according to the way facts are ordered in the to Node.
	 * If a fact is not linked to an effect then the value will be equal to <NULL, NO_INVARIABLE_INDEX>.
	 */
	inline const std::vector<std::pair<const Atom*, InvariableIndex> >& getToNodeEffects() const { return *to_node_effects_; }

	/**
	 * Check if a given term is free, i.e. is not present in any of the preconditions.
	 */
	bool isVariableFree(const Term& term) const;
	
	/**
	 * Get the term which is balanced, there can at most be one! In case of attribute spaces this function will return NULL.
	 */
	inline const Term* getBalancedTerm() const { return balanced_term_; }
	
	/**
	 * Check if the given precondition is removed by any of the effects.
	 * @param precondition The precondition to check.
	 * @return True if the precondition is removed, false otherwise.
	 */
	bool isPreconditionRemoved(const Atom& precondition) const;
	
	/**
	 * After the transition has been updated with the latest effects / preconditions and the from and to nodes are stable and will no longer change,
	 * we will finalise the transition by checking if the static preconditions are satisfied.
	 * @param initial_facts All the initial facts of the problem.
	 * @return True if the transition satisfies the static preconditions, false otherwise.
	 */
	bool finalise(const std::vector<const Atom*>& initial_facts);
	
private:

	/**
	 * Try to create a new transition from the from_node to the to_node. The following assumptions need to be satisfied for this function to work:
	 * 1) There is one and only one shared property being exchanged. This holds for all transitions rules generated by TIM.
	 * 2) No existing bindings exist between the given nodes.
 	 * @param action_step The action to apply to the from node.
	 * @param from_node The start node of the transition.
	 * @param to_node The end node of the transition.
	 * @param property_space_balanced_sets A mapping from every relevant property space to a balanced property set. This set is passed so it can neatly be removed after the function is done.
	 * @return The generated transition or NULL if the action cannot be applied.
	 */
	static Transition* createTransition(const StepPtr action_step, DomainTransitionGraphNode& from_node, DomainTransitionGraphNode& to_node, std::map<const PropertySpace*, BalancedPropertySet*>& property_space_balanced_sets);
	
	/**
	 * Check if any of the facts in @ref facts is mutex with any of the preconditions. In order to be mutex, both need to have a term whose variable domain
	 * is identical to that of the balanced action variable.
	 * @param facts The facts to check the mutex relation with.
	 * @param preconditions The preconditions which are checked if they are mutex with @ref facts.
	 * @param action_step_id The id by which the preconditions are bound.
	 * @param balanced_property_space The property space the facts and the preconditions must be a member of in order to check for mutex relationships.
	 * @param bindings The bindings.
	 * @param balanced_action_variable The variable which is balanced in the transition being considered.
	 * @return True if there exists a mutex relationship, false otherwise.
	 */
	static bool areMutex(const std::vector<BoundedAtom*>& facts, const std::vector<const Atom*>& preconditions, StepID action_step_id, const PropertySpace& balanced_property_space, const Bindings& bindings, const Variable& balanced_action_variable);

	/**
	 * @param step The action, id pair the transition is.
	 * @param from_node The from LTG node.
	 * @param to_node To to LTG node.
	 * @param all_precondition_mappings All the preconditions.
	 * @param from_node_preconditions All the preconditions which are part of the from LTG node, in the order at which the facts occur in that node.
	 * @param all_effect_mappings All the effects.
	 * @param to_node_effects All the effects which are part of the to LTG node, in the order at which the facts occur in that node.
	 * @param free_variables All the terms which are free, i.e. they are not part of any precondition.
	 */
	Transition(StepPtr step, 
	           SAS_Plus::DomainTransitionGraphNode& from_node,
	           SAS_Plus::DomainTransitionGraphNode& to_node,
	           const std::vector< std::pair< const Atom*, InvariableIndex > >& all_precondition_mappings,
	           std::vector< std::pair< const Atom*, InvariableIndex > >& from_node_preconditions,
	           const std::vector< std::pair< const Atom*, InvariableIndex > >& all_effect_mappings,
	           std::vector< std::pair< const Atom*, InvariableIndex > >& to_node_effects,
	           std::vector< std::pair< unsigned int, unsigned int > > & persistent_sets,
	           const std::set<const Term*>& free_variables);
	
	/**
	 * A utility method to perform all the necessary bindings and mappings from this transition to a cloned transition from 
	 * @ref from_node to @ref to_node.
	 * @param step The step of the cloned action to whom all the bindings need to be made.
	 * @param from_node Which is a clone of from_node_.
	 * @param to_node Which is a clone of to_node_.
	 * @param bindings The bindings where all the bindings will be stored.
	 * @return The actual transition which is constructed by cloning this one, this always succeeds or the program quits!
	 */
	Transition* performBindings(MyPOP::StepPtr step, MyPOP::SAS_Plus::DomainTransitionGraphNode& from_node, MyPOP::SAS_Plus::DomainTransitionGraphNode& to_node, int from_fact_ordering[], int to_fact_ordering[], MyPOP::Bindings& bindings) const;

	unsigned int isFactContainedByNode(const Atom& fact, const DomainTransitionGraphNode& node) const;
	
	// The step contains:
	// 1) The action which needs to be executed to make the transition happen and,
	// 2) The step id under which the action's variables are bounded.
	StepPtr step_;

	// The node the transition is going from and to.
	DomainTransitionGraphNode* from_node_;
	DomainTransitionGraphNode* to_node_;

	// All preconditions.
	const std::vector<std::pair<const Atom*, InvariableIndex> >* all_preconditions_;
	
	// For every DTG node we store a <precondition, invariable> pair, NULL if no precondition was found.
	std::vector<std::pair<const Atom*, InvariableIndex> >* from_node_preconditions_;
	
	// All effects.
	const std::vector<std::pair<const Atom*, InvariableIndex> >* all_effects_;
	
	// For every DTG node we store a <precondition, invariable> pair, NULL if no effect was found.
	std::vector<std::pair<const Atom*, InvariableIndex> >* to_node_effects_;

	// The index of the facts in from node and to node which are persistent.
	std::vector<std::pair<unsigned int, unsigned int> >* persistent_sets_;
	
	// An array of action variables which are considered to be 'free'.
	const std::set<const Term*>* free_variables_;
	
	// The term which is balanced.
	const Term* balanced_term_;
};
/*
class RecursivePreconditions
{
	
private:
	const DomainTransitionGraphNode* from_node_;
	const DomainTransitionGraphNode* to_node_;
	const Action* action_;
	
	const std::map<std::pair<const Atom*, InvariableIndex>, std::pair<const Atom*, InvariableIndex> > dtg_node_atoms_to_terminate_conditions;
	
	// Atom's terms link to action variables.
	const std::map<const Atom*, InvariableIndex> dtg_node_atoms_to_recursive_terms;
};
*/
std::ostream& operator<<(std::ostream& os, const Transition& transition);

};

};

#endif // SAS_PLUS_TRANSITION_H
