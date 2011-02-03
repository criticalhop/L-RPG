#ifndef SAS_PLUS_DTG_GRAPH_H
#define SAS_PLUS_DTG_GRAPH_H

#include <vector>
#include <iostream>
#include <map>
#include <set>

#include "dtg_node.h"
#include "../plan_bindings.h"
#include "../manager.h"

namespace MyPOP {

class Atom;
class TypeManager;
class PredicateManager;
class Predicate;
class Object;
class TermManager;
class BindingsPropagator;
	
namespace SAS_Plus {

class BoundedAtom;
class DomainTransitionGraphNode;
class DomainTransitionGraphManager;

/**
 * Bindings class extended to deal with DTG nodes.
 */
class DTGBindings : public MyPOP::Bindings
{
public:

	DTGBindings(const TermManager& term_manager, const BindingsPropagator& propagator);
	DTGBindings(const BindingsFacade& other);

	/**
	 * Check if two DTG nodes can be unified.
	 */
	bool canUnifyDTGNodes(const DomainTransitionGraphNode& node1, const DomainTransitionGraphNode& node2) const;
};


typedef std::pair<const Predicate*, InvariableIndex> IndexedProperty;

/**
 * Property state.
 */
class PropertyState
{
public:
	PropertyState(IndexedProperty property)
	{
		property_.push_back(property);
	}
	
	PropertyState(const std::vector<IndexedProperty>& properties)
	{
		property_.insert(property_.end(), properties.begin(), properties.end());
	}
	
/*	void addProperty(IndexedProperty property)
	{
		property_.push_back(property);
	}
*/
	
	const std::vector<IndexedProperty>& getProperties() const { return property_; }
	
	
	
private:
	std::vector<IndexedProperty> property_;
};

std::ostream& operator<<(std::ostream& os, const PropertyState& property_state);

/**
 * A domain transition graph(DTG) captures the transitions objects of a certain type can make
 * within the planning problem. A DTG is constructed by analysing the domain to see if a combination
 * of predicates are balanced. That is to say, given the initial state and a set of balanced predicates,
 * the number of these predicates in any reachable state will never increase or decrease.
 * From this we can contruct a graph showing the transitions between the propositions in the set
 * we can make. Given a set of objects which are part of this DTG we can use this to calculate
 * heuristics (like Fast-Downward) or use it to find landmarks (like LAMA).
 */
class DomainTransitionGraph : public ManageableObject
{
public:
	DomainTransitionGraph(const MyPOP::SAS_Plus::DomainTransitionGraphManager& dtg_manager, const MyPOP::TypeManager& type_manager, const MyPOP::ActionManager& action_manager, const MyPOP::PredicateManager& predicate_manager, const MyPOP::SAS_Plus::DTGBindings& bindings, const std::vector< const MyPOP::Atom* >& initial_facts);

//	DomainTransitionGraph(const DomainTransitionGraph& dtg);
	
	~DomainTransitionGraph();

	/**
	 * Add a predicate as one of the set which makes a balanced set. The position is the term
	 * of the predicate which is reserved for the objects linked to this DTG. This function can
	 * only be called once.
	 * @param predicate One of the predicates which makes it a balanced set.
	 * @param position The position marks the term which is reserved for objects linked to this DTG.
	 * @param craete_node Create a lifted DTG an attach it to this DTG.
	 */
	void addBalancedSet(const std::vector<PropertyState*>& predicates_to_add, bool create_nodes);
	
	/**
	 * Get the predicates which are present in this DTG.
	 */
	const std::vector<std::pair<const Predicate*, unsigned int> >& getPredicates() const { return predicates_; }
	
	/**
	 * Add an object to this DTG which follows its transition rules.
	 * @param object The object to add as part of this DTG.
	 * TODO: Marked for removal.
	 */
	//void addObject(const Object& object);
	
	/**
	 * Check the initial state for all objects which are part of this DTG and add them.
	 */
	void addObjects();
	
	/**
	 * Remove objects from the domain of the invariants.
	 */
	void removeObjects(const std::set<const Object*>& objects);

	/**
	 * Get all the objects whos transitions are described by this DTG.
	 */
	const std::vector<const Object*>& getObjects() const { return objects_; }

	/**
	 * Add a node to this DTG.
	 * @param dtg_node The DTG node to add to this DTG.
	 * @param added_nodes If this vector is not NULL then all nodes added by this function to the DTG will be added
	 * to this vector as well.
	 */
	bool addNode(DomainTransitionGraphNode& dtg_node, std::vector<DomainTransitionGraphNode*>* added_nodes = NULL);

	/**
	 * Get all nodes already added to this DTG.
	 */
	const std::vector<DomainTransitionGraphNode*>& getNodes() const { return nodes_; }

	/**
	 * Check if two nodes are mutex.
	 */
	bool areMutex(const DomainTransitionGraphNode& dtg_node1, const DomainTransitionGraphNode& dtg_node2) const;
	bool areMutex(const Predicate& predicate1, unsigned int index1, const Predicate& predicate2, unsigned int index2) const;

	/**
	 * Get all nodes which have the given predicate or NULL if no nodes are found.
	 * @param predicate The predicate all nodes searched for are based on.
	 */
	void getNodes(std::vector<DomainTransitionGraphNode*>& dtg_nodes, const Predicate& predicate, unsigned int index) const;
	void getNodes(std::vector<const DomainTransitionGraphNode*>& found_dtg_nodes, const std::vector<const Atom*>& initial_facts, const BindingsFacade& bindings) const;

	/**
	 * Get this DTG's bindings.
	 */
	DTGBindings& getBindings() const { return *bindings_; }
	
	/**
	 * Return the DTG manager.
	 */
	const DomainTransitionGraphManager& getDTGManager() const { return *dtg_manager_; }

	/**
	 * Return the predicate manager.
	 */
	const PredicateManager& getPredicateManager() const { return *predicate_manager_; }

	/**
	 * Return the term manager.
	 */
	const TermManager& getTermManager() const { return *dtg_term_manager_; }

	/**
	 * Check if the given index of the given predicate is an invariable variable. The predicates will only
	 * be checked if they have the same name and arity. The types do not need to match exactly, as long
	 * as the types of the given predicate are more specific or equal to the the types of the DTG's predicate.
	 * @param predicate The Predicate to search for.
	 * @param index The index to check.
	 */
	bool isValidPredicateIndex(const Predicate& predicate, unsigned int index) const;
	
	/**
	 * Create a new DTG node with the given atom and add bind t to this DTG's bindings. The node is not added though!
	 * @param atom The atom to create the lifted DTG node from.
	 */
	DomainTransitionGraphNode* createDTGNode(const Atom& atom, unsigned int index);
	
	/**
	 * Remove a node from the DTG node.
	 * @param dtg_node The DTG node to remove.
	 */
	void removeNode(const DomainTransitionGraphNode& dtg_node);
	
	/**
	 * Find all the nodes which can be unified with the given atom and its bindings.
	 * @param dtg_nodes All nodes which can be unified are added to this list.
	 * @param step_id The step id which has been used to bind the atom's variables.
	 * @param atom The bounded atom.
	 * @param bindings The bindings which are used to bind the atom's bindings.
	 * @param index The index at which the variable should be invariable in the found DTG node. If this variable
	 * is equal to std::numeric_limits<unsigned int>::max() this constraint isn't checked.
	 */
	void getNodes(std::vector<const DomainTransitionGraphNode*>& dtg_nodes, StepID step_id, const Atom& atom, const BindingsFacade& bindings, unsigned int index = std::numeric_limits<unsigned int>::max()) const;
	
	/**
	 * Identify subgraphs within a DTG and split those up into seperate graphs.
	 * @param subgraphs All identified subgraphs will be inserted into this list.
	 */
	void identifySubGraphs(std::vector<DomainTransitionGraph*>& subgraphs) const;
	
	/**
	 * After subgraphs have been detected - every subgraph containing a unique type - propagate this
	 * information to the other graphs. If - for example - there are two different types of trucks we
	 * need to check what the impact of this devision has on all transitions which have trucks in their
	 * preconditions.
	 *
	 * The DTG for package: (at package location) <-> (in package truck)
	 *
	 * Needs to be modified to account for the different types of trucks in the domain. If there are
	 * more types of trucks it indicates that the road network (i.e. the connect predicates) do not
	 * allow a location to be reached from all other locations. By checking the preconditions of
	 * the transitions we can determine if a node needs to be split. In this case, because (at truck location)
	 * appears as a precondition in the (unload package truck location) operator we need to split the
	 * (in package truck) node.
	 *
	 * The variable to split is the invariable domain of the splitted node it is compared to. In this
	 * case this is the variable truck (since that is the invariable in the DTGs for trucks). The node
	 * is copied for every possible type of truck and the transitions are updated accordingly.
	 *
	 * After splitting these nodes, the DomainTransitionGraphManager::generateTransitionGraphs, will
	 * record these changes and continue iterating over all splitted DTGs until no further changes
	 * occur.
	 * @param split_graphs These are all the graphs that have been split, thus the graphs we need to
	 * compare the preconditions of this DTG with.
	 */
	void splitNodes(const std::map<DomainTransitionGraph*, std::vector<DomainTransitionGraph*>* >& split_graphs);
	void splitNodes(const std::vector<DomainTransitionGraph*>& split_graphs);
	
	bool isSupported(unsigned int id, const Atom& atom, const BindingsFacade& bindings) const;
	
	void removeUnsupportedTransitions();

	friend std::ostream& operator<<(std::ostream& os, const DomainTransitionGraph& dtg);
	
	// Merge the predicates.
	void mergePredicates(const DomainTransitionGraph& other);
	
	/**
	 * After nodes are split, reestablish the transitions of their lifted parents.
	 * @param new_transitions The grounded transitions which have been made to link to new grounded transitions.
	 * @param new_nodes A mapping from the new grounded nodes to the lifted nodes they were derived from.
	 * A transition can only be established between two nodes if the transition was present between their lifted
	 * parents. This prevents transitions being added between nodes where no such transition existed before.
	 */
//	void fixTransitions(const std::vector< const MyPOP::SAS_Plus::Transition* >& new_transitions, std::map< MyPOP::SAS_Plus::DomainTransitionGraphNode*, const MyPOP::SAS_Plus::DomainTransitionGraphNode* >& new_nodes);
	
	/**
	 * Try all possible transitions on the set of nodes in this DTG and add those that are possible.
	 */
	void reestablishTransitions();
	
	void establishTransitions();

	
private:
	/**
	 * Given a balanced set of predicates, update the mutex relationships by making all the predicates in the set 
	 * mutex with each other.
	 */
	void updateMutexRelations(const std::vector<PropertyState*>& predicates_to_add);
	
	const DomainTransitionGraphManager* dtg_manager_;

	// When we split DTG nodes up we have a need for new atoms for every node. To manage the
	// terms we add them to this term manager (and remove them as well when needed.
	TermManager* dtg_term_manager_;
	
	const ActionManager* action_manager_;

	// The predicate manager.
	const PredicateManager* predicate_manager_;

	// To propagate changes made to the DTGs we keep track of all bindings between them and propagate changes
	// as necessary.
	DTGBindings* bindings_;
	
	const std::vector<const Atom*>* initial_facts_;

	// The nodes of this DTG.
	std::vector<DomainTransitionGraphNode*> nodes_;

	// The objects which share this DTG.
	std::vector<const Object*> objects_;

	// To create a DTG a set of predicates are combined to construct a 'balanced set', i.e.
	// taken all the effects of all actions involving these predicates there will always be
	// a single value true in any given state for the above objects. The int is the parameter
	// number of the predicate reserved for the objects linked to this DTG. Between any transition
	// the object on the given position will always be the same; e.g. (at PACKAGE ?loc) -> (in PACKAGE ?truck).
	// Read: Exhibiting Knowledge in Planning Problems to Minimize State Encoding Length
	// by Stefan Edelkamp and Malte Helmert.
	std::vector<IndexedProperty> predicates_;

	// Mutex relations between the predicates.
	std::map<IndexedProperty, std::set<IndexedProperty>*> mutex_map_;

	// Most specific type of the invariable object.
	const Type* type_;
};

std::ostream& operator<<(std::ostream& os, const DomainTransitionGraph& dtg);

};

};

#endif // SAS_PLUS_DTG_GRAPH_H