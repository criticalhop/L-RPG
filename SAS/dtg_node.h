#ifndef SAS_PLUS_DTG_NODE_H
#define SAS_PLUS_DTG_NODE_H

#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <limits>

#include "../plan_types.h"
#include "dtg_types.h"

namespace MyPOP {

class Atom;
class Action;
class VariableDomain;
class BindingsFacade;

namespace SAS_Plus {

class DomainTransitionGraph;
class Transition;
class BoundedAtom;

/**
 * All possible values a state variable can take are stored in DTG nodes. We use a lifted
 * SAS+ representation so every node might correspond to multiple possible values a state
 * variable can be assigned to. The possible values depend on:
 * 1) The atoms (thus predicates) of this node.
 * 2) The bindings enforced on the variables of these atoms. These can be retrieved from the
 * bindings object and the VariableDomains assigned to the variables.
 */
class DomainTransitionGraphNode
{
public:
	DomainTransitionGraphNode(DomainTransitionGraph& dtg, unsigned int unique_id);

	/**
	 * Make a copy of an existing DTG node. We inherrit the same domain for the variables and atom,
	 * but it is in no way linked to the node it is copied from. All the transitions from the given
	 * node are copied as are all transitions to the other node.
	 */
	DomainTransitionGraphNode(const DomainTransitionGraphNode& dtg_node, bool copy_transitions = true);
	
	/**
	 * Make a copy of an existing DTG node. We inherrit the same domain for the variables and atom,
	 * but it is in no way linked to the node it is copied from. None of the transitions are copied
	 * but the node is linked given DTG, but not added to it!
	 */
	DomainTransitionGraphNode(const DomainTransitionGraphNode& dtg_node, DomainTransitionGraph& dtg);
	
	~DomainTransitionGraphNode();

	/**
	 * Add an atom to this node.
	 */
	void addAtom(const Atom& atom, StepID id, InvariableIndex index);
	void addAtom(BoundedAtom* bounded_atom, InvariableIndex index);

	/**
	 * Remove atoms (lazy remove).
	 */
	void removeAtom(const BoundedAtom& bounded_atom);

	/**
	 * Get the index of the invariable variable of one of the node's atoms.
	 * @param atom An atom contained by atoms_.
	 * @return The index of the variable which is invariable.
	 */
	InvariableIndex getIndex(const BoundedAtom& atom) const;
	
	/**
	 * Get the index of the invariable variable of a atom which is not part of this DTG node.
	 * For example, all transitions leading to and from this node share variable domains with
	 * this node. This function returns the index of the variable which is invariable.
	 * @param id The step id of the given atom.
	 * @param atom The atom which is linked to this DTG node.
	 * @return The index of the variable which is invariable.
	 */
	InvariableIndex getIndex(StepID id, const Atom& atom) const;

	/**
	 * Add a transition from this node to to_node, without checking for static preconditions.
	 */
	bool addTransition(const std::vector<BoundedAtom>& enablers, const Action& action, DomainTransitionGraphNode& to_node);

	/**
	 * Add a transition to this node.
	 */
	bool addTransition(const Transition& transition, bool update_possible_transitions);

	/**
	 * Get the atoms linked to this node.
	 */
	const std::vector<BoundedAtom*>& getAtoms() const { return atoms_; }
	
	/**
	 * Get the DTG this node is part of.
	 */
	const DomainTransitionGraph& getDTG() const { return *dtg_; }

	/**
	 * Get all transition from this node.
	 */
	const std::vector<const Transition*>& getTransitions() const { return transitions_; }
	std::vector<const Transition*>& getTransitionsNonConst() { return transitions_; }
	
	/**
	 * Remove a transition from this node.
	 */
	bool removeTransition(const Transition& transition);
	void removeTransitions();

	/**
	 * Two DTG nodes are equal if they have the same atoms and both atoms their variable domains contain the
	 * same values. They can be separate objects!
	 */
	bool operator==(const DomainTransitionGraphNode& dtg_node) const;

	friend std::ostream& operator<<(std::ostream&, const DomainTransitionGraphNode& node);

	/**
	 * Ground out a specific variable of all Atoms. All possible instantiations are produced and stored in the given vector. This node
	 * remains unchanged, to replace this node it has to be removed from the DTG and all the produced nodes added. Transitions are not
	 * copied or affected.
	 * @param ground_nodes This will contain the grounded out copies of this node.
	 * @param variable_to_ground The variable which needs to be grounded, membership is tested through pointer checking.
	 * @return true if at least one grounded node was produced, false otherwise.
	 */
	bool groundVariable(std::vector<DomainTransitionGraphNode*>& grounded_nodes, const VariableDomain& variable_to_ground) const;
	
	/**
	 * A node is invalid if one of its atoms's domains is empty.
	 */
	bool isValid() const;

	void removeUnsupportedTransitions();
	
	void print(std::ostream& os) const;
	
	bool isSupported(unsigned int id, const Atom& atom, const BindingsFacade& bindings) const;

	/**
	 * Merge with the given DTG node. The atoms of the other node will be copied over and the predicates
	 * associated with the DTG will be altered accordingly.
	 * @param other The DTG node which will be merged with this one.
	 * @return true if new atoms were added to this node, false otherwise.
	 */
	bool merge(const DomainTransitionGraphNode& other);
	
	void getPossibleActions(std::vector< const MyPOP::Action* >& possible_actions, const MyPOP::SAS_Plus::DomainTransitionGraphNode& dtg_node) const;


private:

	/**
	 * Utility function of the copy constructor. Copy all the atoms to the new copy of the DTG node.
	 */
	void copyAtoms(const DomainTransitionGraphNode& dtg_node);
	
	// The DTG this node is part of.
	DomainTransitionGraph* dtg_;

	// The value of this node.
	std::vector<BoundedAtom*> atoms_;

	// The set of transitions from this node to any other node.
	std::vector<const Transition*> transitions_;

	// To create a DTG a set of predicates are combined to construct a 'balanced set', i.e.
	// taken all the effects of all actions involving these predicates there will always be
	// a single value true in any given state for the above objects. The int is the parameter
	// number of the predicate reserved for the objects linked to this DTG. Between any transition
	// the object on the given position will always be the same; e.g. (at PACKAGE ?loc) -> (in PACKAGE ?truck).
	// Read: Exhibiting Knowledge in Planning Problems to Minimize State Encoding Length
	// by Stefan Edelkamp and Malte Helmert.
	std::map<const BoundedAtom*, InvariableIndex> indexes_;
	
	std::vector<unsigned int> unique_ids_;
	//std::map<unsigned int, std::vector<const Action* >* > possible_actions_;
	std::multimap<unsigned int, const Action*> possible_actions_;
};

std::ostream& operator<<(std::ostream&, const DomainTransitionGraphNode& node);

};

};

#endif // SAS_PLUS_DTG_NODE_H