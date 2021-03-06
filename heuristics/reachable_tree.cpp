#include "reachable_tree.h"

#include <limits>

#include "dtg_reachability.h"
#include <predicate_manager.h>
#include "equivalent_object_group.h"

//#define MYPOP_SAS_PLUS_REACHABLE_TREE_COMMENT
//#define MYPOP_SAS_PLUS_REACHABLE_TREE_DEBUG
#include "fact_set.h"

namespace MyPOP {

namespace REACHABILITY {

ReachableTreeNode::ReachableTreeNode(ReachableTree& tree, ReachableTreeNode* parent, unsigned int level, const ReachableFact& reachable_fact)
	: tree_(&tree), parent_(parent), level_(level), max_level_of_children_(level), reachable_fact_(&reachable_fact), last_iteration_updated_(std::numeric_limits<unsigned int>::max()), has_been_processed_(false)
{
	assert (tree_ != NULL);
	if (level_ + 1 == tree_->getMaxDepth()) tree.addNewLeaf(*this);
	// assert (&reachable_fact != NULL);
}

ReachableTreeNode::~ReachableTreeNode()
{
	// Delete all children first!
	for (std::vector<ReachableTreeNode*>::const_reverse_iterator ri = children_.rbegin(); ri != children_.rend(); ri++)
	{
		assert (*ri != NULL);
		delete *ri;
	}
}

void ReachableTreeNode::printCompleteTree(std::ostream& os) const
{
	os << *tree_ << std::endl;
}

bool ReachableTreeNode::updateReachableFact()
{
	if (!reachable_fact_->isMarkedForRemoval()) return false;
	reachable_fact_ = &reachable_fact_->getReplacement();
	return true;
}

ConstReachableTreeIterator ReachableTreeNode::cbegin() const
{
	assert (tree_ != NULL);
	assert (level_ == tree_->getMaxDepth() - 1);
	return ConstReachableTreeIterator(this);
}

ConstReachableTreeIterator ReachableTreeNode::cend() const
{
	return ConstReachableTreeIterator(NULL);
}

ReachableTreeIterator ReachableTreeNode::begin()
{
	return ReachableTreeIterator(this);
}

ReachableTreeIterator ReachableTreeNode::end() const
{
	return ReachableTreeIterator(NULL);
}

unsigned int ReachableTreeNode::addFact(unsigned int level, const ReachableFact& reachable_fact)
{
#ifdef MYPOP_SAS_PLUS_REACHABLE_TREE_COMMENT
	std::cout << "Try to add " << reachable_fact << " at level " << level << " to the node: " << *this << std::endl;
#endif
	
	// Check if the tree is deep enough to accommodate the new reachable fact.
	if (max_level_of_children_ + 1 < level)
	{
#ifdef MYPOP_SAS_PLUS_REACHABLE_TREE_COMMENT
		std::cout << "* Tree is not deep enough :(." << std::endl;
#endif
		return std::numeric_limits<unsigned int>::max();
	}
	
	// Check if the fact should be added here as a child.
	if (level_ + 1 == level)
	{
		if (!canSatisfyConstraints(reachable_fact, level))
		{
#ifdef MYPOP_SAS_PLUS_REACHABLE_TREE_COMMENT
			std::cout << "* Tree deep enough, but constraints cannot be satisfied :(." << std::endl;
#endif
			return std::numeric_limits<unsigned int>::max();
		}
		
		ReachableTreeNode* new_reachable_node = new ReachableTreeNode(*tree_, this, level, reachable_fact);
		
		children_.push_back(new_reachable_node);
#ifdef MYPOP_SAS_PLUS_REACHABLE_TREE_COMMENT
		std::cout << "* Succes! :D Number of leaves: " << tree_->getLeaves().size() << std::endl;
#endif
		if (max_level_of_children_ < level_ + 1)
		{
			max_level_of_children_ = level_ + 1;
		}
		
		// Try to build the next layer (if possible!).
		if (level + 1 == tree_->getMaxDepth())
		{
#ifdef MYPOP_SAS_PLUS_REACHABLE_TREE_COMMENT
			std::cout << "* Reached the last layer, we're done!" << std::endl;
#endif
			return level;
		}
		
		const std::list<ReachableFact*>& potential_new_facts = tree_->getFactsAtLevel(level + 1);
#ifdef MYPOP_SAS_PLUS_REACHABLE_TREE_COMMENT
		std::cout << "Found " << potential_new_facts.size() << " fact for the " << (level + 1) << "th layer!" << std::endl;
#endif
		for (std::list<ReachableFact*>::const_iterator ci = potential_new_facts.begin(); ci != potential_new_facts.end(); ci++)
		{
			assert (!(*ci)->isMarkedForRemoval());
			unsigned int deepest_level = new_reachable_node->addFact(level + 1, **ci);
			if (deepest_level != std::numeric_limits<unsigned int>::max() &&
			    deepest_level > max_level_of_children_)
			{
				max_level_of_children_ = deepest_level;
			}
		}
		return max_level_of_children_;
	}
	else
	{
		for (std::vector<ReachableTreeNode*>::const_iterator ci = children_.begin(); ci != children_.end(); ci++)
		{
			unsigned int added_level = (*ci)->addFact(level, reachable_fact);
			if (added_level != std::numeric_limits<unsigned int>::max() &&
			    added_level > max_level_of_children_)
			{
				max_level_of_children_ = added_level;
			}
		}
		return max_level_of_children_;
	}
	
	return std::numeric_limits<unsigned int>::max();
}
/*
unsigned int ReachableTreeNode::addFact(unsigned int level, const ReachableFact& reachable_fact, const std::vector<std::vector<std::pair<unsigned int, unsigned int> >** >& constraints_set)
{
#ifdef MYPOP_SAS_PLUS_REACHABLE_TREE_COMMENT
	std::cout << "Try to add " << reachable_fact << " at level " << level << " to the node: " << *this << std::endl;
#endif
	
	// Check if the tree is deep enough to accommodate the new reachable fact.
	if (max_level_of_children_ + 1 < level)
	{
#ifdef MYPOP_SAS_PLUS_REACHABLE_TREE_COMMENT
		std::cout << "* Tree is not deep enough :(." << std::endl;
#endif
		return std::numeric_limits<unsigned int>::max();
	}
	
	// Check if the fact should be added here as a child.
	if (level_ + 1 == level)
	{
		if (!canSatisfyConstraints(reachable_fact, level, constraints_set))
		{
#ifdef MYPOP_SAS_PLUS_REACHABLE_TREE_COMMENT
			std::cout << "* Tree deep enough, but constraints cannot be satisfied :(." << std::endl;
#endif
			return std::numeric_limits<unsigned int>::max();
		}
		
		ReachableTreeNode* new_reachable_node = new ReachableTreeNode(*tree_, this, level, reachable_fact);
		
		children_.push_back(new_reachable_node);
#ifdef MYPOP_SAS_PLUS_REACHABLE_TREE_COMMENT
		std::cout << "* Succes! :D Number of leaves: " << tree_->getLeaves().size() << std::endl;
#endif
		if (max_level_of_children_ < level_ + 1)
		{
			max_level_of_children_ = level_ + 1;
		}
		
		// Try to build the next layer (if possible!).
		if (level + 1 == tree_->getMaxDepth())
		{
#ifdef MYPOP_SAS_PLUS_REACHABLE_TREE_COMMENT
			std::cout << "* Reached the last layer, we're done!" << std::endl;
#endif
			return level;
		}
		
		const std::list<ReachableFact*>& potential_new_facts = tree_->getFactsAtLevel(level + 1);
#ifdef MYPOP_SAS_PLUS_REACHABLE_TREE_COMMENT
		std::cout << "Found " << potential_new_facts.size() << " fact for the " << (level + 1) << "th layer!" << std::endl;
#endif
		for (std::list<ReachableFact*>::const_iterator ci = potential_new_facts.begin(); ci != potential_new_facts.end(); ci++)
		{
			assert (!(*ci)->isMarkedForRemoval());
			unsigned int deepest_level = new_reachable_node->addFact(level + 1, **ci, constraints_set);
			if (deepest_level != std::numeric_limits<unsigned int>::max() &&
			    deepest_level > max_level_of_children_)
			{
				max_level_of_children_ = deepest_level;
			}
		}
		return max_level_of_children_;
	}
	else
	{
		for (std::vector<ReachableTreeNode*>::const_iterator ci = children_.begin(); ci != children_.end(); ci++)
		{
			unsigned int added_level = (*ci)->addFact(level, reachable_fact, constraints_set);
			if (added_level != std::numeric_limits<unsigned int>::max() &&
			    added_level > max_level_of_children_)
			{
				max_level_of_children_ = added_level;
			}
		}
		return max_level_of_children_;
	}
	
	return std::numeric_limits<unsigned int>::max();
}
*/
void ReachableTreeNode::addChild(ReachableTreeNode* reachable_tree_node)
{
	children_.push_back(reachable_tree_node);
	if (max_level_of_children_ < reachable_tree_node->max_level_of_children_)
	{
		max_level_of_children_ = reachable_tree_node->max_level_of_children_;
	}
}

bool ReachableTreeNode::canSatisfyConstraints(const ReachableFact& reachable_fact, unsigned int level) const
{
//	std::cout << "[ReachableTreeNode::canSatisfyConstraints] " << reachable_fact << "; level=" << level << std::endl;
	const std::vector<std::vector<std::pair<unsigned int, unsigned int> >* >& constraints = tree_->getReachableSet().getConstraints();
/*
	std::cout << "Constraints to be imposed: " << std::endl;
	for (std::vector<std::vector<std::pair<unsigned int, unsigned int> >* >::const_iterator ci = constraints.begin(); ci != constraints.end(); ++ci)
	{
		std::vector<std::pair<unsigned int, unsigned int> >* constraint = *ci;
		std::cout << std::distance(constraints.begin(), ci) << ", ";
		for (std::vector<std::pair<unsigned int, unsigned int> >::const_iterator ci = constraint->begin(); ci != constraint->end(); ++ci)
		{
			std::cout << (*ci).first << ", " << (*ci).second << ", ";
		}
		std::cout << "." << std::endl;
	}
	
	for (std::vector<EquivalentObjectGroup*>::const_iterator ci = reachable_fact.getTermDomains().begin(); ci != reachable_fact.getTermDomains().end(); ++ci)
	{
		std::cout << *ci << " ";
	}
	std::cout << "." << std::endl;
*/
	for (unsigned int reachable_fact_term_index = 0; reachable_fact_term_index < reachable_fact.getTermDomains().size(); ++reachable_fact_term_index)
	{
		unsigned int fact_index = (*constraints[level])[reachable_fact_term_index].first;
		unsigned int variable_index = (*constraints[level])[reachable_fact_term_index].second;
		
		const ReachableTreeNode* cur_node = this;
		
		//std::cout << "Check that the constraint of " << reachable_fact << "; level=" << level << "." << std::endl;
		//std::cout << "The " << reachable_fact_term_index << "th variable should match up with the " << fact_index << " fact and its " << variable_index << "th variable." << std::endl;
		//std::cout << "Fact in question: " << cur_node->getReachableFact() << std::endl;
		
		// Get the correct node to match the fact index.
		//std::cout << "Search for the " << fact_index << " fact!" << std::endl;
		
		// If the constraint is part of the fact level itself it is only constrained by itself and we can ignore it.
		if (fact_index == level)
		{
			continue;
		}
		
		while (fact_index != cur_node->getLevel())
		{
			//std::cout << "! " << cur_node->getLevel() << std::endl;
			cur_node = cur_node->getParent();
			assert (cur_node != NULL);
		}
		
		//for (std::vector<EquivalentObjectGroup*>::const_iterator ci = cur_node->getReachableFact().getTermDomains().begin(); ci != cur_node->getReachableFact().getTermDomains().end(); ++ci)
		//{
		//	std::cout << *ci << " ";
		//}
		//std::cout << "." << std::endl;
		
		// Check if the relationship holds.
		if (&reachable_fact.getTermDomain(reachable_fact_term_index) != &cur_node->getReachableFact().getTermDomain(variable_index))
		{
			return false;
		}
	}
	return true;
}

/*
bool ReachableTreeNode::canSatisfyConstraints(const ReachableFact& reachable_fact, unsigned int level, const std::vector<std::vector<std::pair<unsigned int, unsigned int> >** >& constraints_set) const
{
	std::vector<std::pair<unsigned int, unsigned int> >** constraints = constraints_set[level];
	for (unsigned int i = 0; i < reachable_fact.getAtom().getArity(); i++)
	{
		std::vector<std::pair<unsigned int, unsigned int> >* variable_constraints = constraints[i];
		
		for (std::vector<std::pair<unsigned int, unsigned int> >::const_iterator ci = variable_constraints->begin(); ci != variable_constraints->end(); ci++)
		{
			unsigned int fact_index = (*ci).first;
			unsigned int variable_index = (*ci).second;
			
			// Get the correct node to match the fact index.
			const ReachableTreeNode* cur_node = this;
			while (fact_index != cur_node->getLevel())
			{
				assert (cur_node != NULL);
				cur_node = cur_node->getParent();
			}
			
			// Check if the relationship holds.
			if (&reachable_fact.getTermDomain(i) != &cur_node->getReachableFact().getTermDomain(variable_index))
			{
				return false;
			}
		}
	}
	return true;
}
*/
void ReachableTreeNode::sanityCheck() const
{
	for (std::vector<ReachableTreeNode*>::const_iterator ci = children_.begin(); ci != children_.end(); ci++)
	{
		const ReachableTreeNode* rtn = *ci;
		assert (rtn->getParent() == this);
		rtn->sanityCheck();
	}
}

//void ReachableTreeNode::updateChildren(const ReachableSet& reachable_set, const std::vector<std::vector<std::pair<unsigned int, unsigned int> >** >& constraints_set)
void ReachableTreeNode::updateChildren(const ReachableSet& reachable_set)
{
	// Remove all children which are redundant.
	for (std::vector<ReachableTreeNode*>::reverse_iterator ri = children_.rbegin(); ri != children_.rend(); ri++)
	{
		ReachableTreeNode* child = *ri;
		assert (&child->getTree() == tree_);
		if (child->getReachableFact().isMarkedForRemoval())
		{
			delete child;
			children_.erase(ri.base() - 1);
		}
		else
		{
//			child->updateChildren(reachable_set, constraints_set);
			child->updateChildren(reachable_set);
			assert (&child->getTree() == tree_);
		}
	}

	// Next add all the nodes which have been overlooked - unless this is a leaf node of course.
	if (level_ + 1 != tree_->getMaxDepth())
	{
		for (std::list<ReachableFact*>::const_iterator ci = reachable_set.getReachableSets()[level_ + 1]->begin(); ci != reachable_set.getReachableSets()[level_ + 1]->end(); ci++)
		{
			ReachableFact* reachable_fact = *ci;
			assert (!reachable_fact->isMarkedForRemoval());
			
			// Check if this reachable fact is already part of one of the children.
			bool is_child_of = false;
			for (std::vector<ReachableTreeNode*>::const_iterator ci = children_.begin(); ci != children_.end(); ci++)
			{
				ReachableTreeNode* child = *ci;
				if (&child->getReachableFact() == reachable_fact)
				{
					is_child_of = true;
					break;
				}
			}
			
			// If it is not part of a child yet, try to add it!
			if (!is_child_of)
			{
				addFact(level_ + 1, *reachable_fact);
				// Check grounded properties.
/*				bool ground_properties_satisfied = true;
				for (unsigned int i = 0; i < reachable_fact->getAtom().getArity(); i++)
				{
					if (reachable_set.getFactsSet()[level_ + 1]->isGrounded(i) &&
					   (
					      !reachable_fact->getTermDomain(i).isGrounded() ||
					      &reachable_fact->getTermDomain(i).getEquivalentObjects()[0]->getObject() != reachable_set.getFactsSet()[level_ + 1]->getVariableDomain(i)[0]
					   )
					)
					{
						ground_properties_satisfied = false;
						break;
					}
				}
				
				if (ground_properties_satisfied)
				{

					addFact(level_ + 1, *reachable_fact, constraints_set);
				}
*/
			}
		}
	}
}

std::ostream& operator<<(std::ostream& os, const ReachableTreeNode& reachable_tree_node)
{
	assert (reachable_tree_node.reachable_fact_ != NULL);
	//assert (!reachable_tree_node.reachable_fact_->isMarkedForRemoval());
	
	os << *reachable_tree_node.reachable_fact_;

	os << "Parents: ";
	const ReachableTreeNode* current_node = &reachable_tree_node;
	while (current_node != NULL)
	{
		os << "+ [" << current_node->level_ << "] " << *current_node->reachable_fact_ << "(" << current_node->reachable_fact_ << ")" << std::endl;
		current_node = current_node->getParent();
	}	
	os << "[" << reachable_tree_node.level_ << "/" << reachable_tree_node.max_level_of_children_ << "] " << *reachable_tree_node.reachable_fact_ << "(" << reachable_tree_node.reachable_fact_ << ")" << std::endl;
	os << "Children:" << std::endl;
	for (std::vector<ReachableTreeNode*>::const_iterator ci = reachable_tree_node.children_.begin(); ci != reachable_tree_node.children_.end(); ci++)
	{
		os << "+ [" << (*ci)->level_ << "] " << *(*ci)->reachable_fact_ << "(" << (*ci)->reachable_fact_ << ")" << std::endl;
	}
	for (std::vector<ReachableTreeNode*>::const_iterator ci = reachable_tree_node.children_.begin(); ci != reachable_tree_node.children_.end(); ci++)
	{
		os << **ci << std::endl;
	}

	return os;
}
/*
ReachableTree::ReachableTree(const ReachableSet& reachable_set, const std::vector<std::vector<std::pair<unsigned int, unsigned int> >** >& constraints_set)
	: reachable_set_(&reachable_set), constraints_set_(&constraints_set), root_node_(NULL), cache_is_valid_(false), cached_size_(0)
{
#ifdef MYPOP_SAS_PLUS_REACHABLE_TREE_DEBUG
	sanityCheck();
#endif
}
*/
ReachableTree::ReachableTree(const ReachableSet& reachable_set)
	: reachable_set_(&reachable_set), root_node_(NULL), cache_is_valid_(false), cached_size_(0)
{
#ifdef MYPOP_SAS_PLUS_REACHABLE_TREE_DEBUG
	sanityCheck();
#endif
}

ReachableTree::~ReachableTree()
{
	if (root_node_ != NULL) delete root_node_;
}

void ReachableTree::addFact(unsigned int level, const ReachableFact& reachable_fact)
{
#ifdef MYPOP_SAS_PLUS_REACHABLE_TREE_DEBUG
	sanityCheck();
#endif

#ifdef MYPOP_SAS_PLUS_REACHABLE_TREE_COMMENT
	std::cout << "[ReachableTree::addFact] Level=" << level << " add=" << reachable_fact << std::endl;
	std::cout << " === Tree facts " << this << " === " << std::endl;
	std::cout << *reachable_set_ << std::endl;
	std::cout << " /// Tree facts /// " << std::endl;
#endif

	// Add a new root node!
	if (level == 0)
	{
		assert (root_node_ == NULL);
		root_node_ = new ReachableTreeNode(*this, NULL, level, reachable_fact);
#ifdef MYPOP_SAS_PLUS_REACHABLE_TREE_COMMENT
		std::cout << " === New Root Node created! ===" << std::endl;
		std::cout << *root_node_;
		std::cout << " /// New Root Node created! ///" << std::endl;
#endif
		// Check if we can add more facts.
		if (getMaxDepth() != level + 1)
		{
			for (std::list<ReachableFact*>::const_iterator ci = reachable_set_->getReachableSets()[level + 1]->begin(); ci != reachable_set_->getReachableSets()[level + 1]->end(); ci++)
			{
//				root_node_->addFact(level + 1, **ci, *constraints_set_);
				root_node_->addFact(level + 1, **ci);
			}
		}
	}
	else
	{
		assert (root_node_ != NULL);
//		root_node_->addFact(level, reachable_fact, *constraints_set_);
		root_node_->addFact(level, reachable_fact);
	}
#ifdef MYPOP_SAS_PLUS_REACHABLE_TREE_DEBUG
	sanityCheck();
#endif
//	std::cout << "Added: " << reachable_fact << "[level=" << level << "] to " << *this << std::endl;
}

void ReachableTree::equivalencesUpdated(unsigned int iteration, const std::vector<ReachableTree*>& reachable_trees)
{
	// After finding all the new facts which can be achieved the equivalences are updated. This allows us
	// to mark which facts have been processed and which still need to be processed.
	if (iteration != 0)
	{
		for (unsigned int i = 0; i < cached_size_; i++)
		{
			leaf_nodes_[i]->markAsProcessed();
		}
	}
	cache_is_valid_ = false;
//	std::cout << "Reset cache!" << std::endl;

#ifdef MYPOP_SAS_PLUS_REACHABLE_TREE_DEBUG
	sanityCheck();
#endif

	// Update the leaf nodes by removing all those which are out of date and adding those which are not.
	for (std::vector<ReachableTreeNode*>::reverse_iterator ri = leaf_nodes_.rbegin(); ri != leaf_nodes_.rend(); ri++)
	{
		ReachableTreeNode* leaf_node = *ri;

		// If any node from the leaf to the root is due to be removed, remove the leaf node!
		for (ConstReachableTreeIterator crti = leaf_node->cbegin(); crti != leaf_node->cend(); crti++)
		{
			if ((*crti).getReachableFact().isMarkedForRemoval())
			{
				leaf_nodes_.erase(ri.base() - 1);
				break;
			}
		}
	}
	
	// We know the root of the tree is not to be removed. All its children and its children's children which are out of date need to be removed and 
	// we need to check the reachable facts set to make sure did not forget to include any of its nodes. We have removed all trees which contained a 
	// root which was not up to date, by doing so we might have "lost" nodes which must be included in this tree. Therefore, by going through the set 
	// of all reachable facts, we need to reconstruct these potential lost nodes.
	// Another way of doing it is by merging the trees that are to be removed with those which stay and copying all the relevant information.
//	root_node_->updateChildren(*reachable_set_, *constraints_set_);
	root_node_->updateChildren(*reachable_set_);
	
	// Determine which leaf nodes are already processed and do not need to be processed again.
	for (unsigned int leaf_index = 0; leaf_index < leaf_nodes_.size(); ++leaf_index)
	{
		ReachableTreeNode* leaf = leaf_nodes_[leaf_index];
		if (leaf->hasBeenProcessed()) continue;
		
		for (std::vector<ReachableTree*>::const_iterator ci = reachable_trees.begin(); ci != reachable_trees.end(); ci++)
		{
			const ReachableTree* reachable_tree = *ci;

			if (reachable_tree != this &&
					reachable_tree->root_node_->getReachableFact().isMarkedForRemoval() && 
					&reachable_tree->root_node_->getReachableFact().getReplacement() == &root_node_->getReachableFact())
			{
				// Compare this tree's processed leafs to those leafs which are not yet marked as processed yet and check if they are the same.
				for (unsigned int other_leaf_index = 0; other_leaf_index < reachable_tree->cached_size_; other_leaf_index++)
				{
					const ReachableTreeNode* other_leaf = reachable_tree->leaf_nodes_[other_leaf_index];
					
					// Check if these leafs pass through the same path.
					bool paths_are_identical = true;
					ReachableTreeNode* leaf_copy = leaf;
					for (unsigned int i = 0; i < getMaxDepth(); i++)
					{
						if (&other_leaf->getReachableFact().getReplacement() != &leaf_copy->getReachableFact())
						{
							paths_are_identical = false;
							break;
						}
						leaf_copy = leaf_copy->getParent();
						other_leaf = other_leaf->getParent();
					}
					
					if (paths_are_identical)
					{
						leaf->markAsProcessed();
						break;
					}
				}
				if (leaf->hasBeenProcessed()) break;
			}
		}
	}
	
#ifdef MYPOP_SAS_PLUS_REACHABLE_TREE_DEBUG
	// Check there are no duplicates!
	if (!leaf_nodes_.empty())
	{
		for (unsigned int i = 0; i < leaf_nodes_.size() - 1; i++)
		{
			for (unsigned int j = i + 1; j < leaf_nodes_.size(); j++)
			{
				const ReachableTreeNode* node1 = leaf_nodes_[i];
				assert (!node1->getReachableFact().isMarkedForRemoval());

				const ReachableTreeNode* node2 = leaf_nodes_[j];
				assert (!node2->getReachableFact().isMarkedForRemoval());

				bool are_identical = true;
				for (unsigned int depth = 0; depth < getMaxDepth(); depth++)
				{
					if (&node1->getReachableFact() != &node2->getReachableFact())
					{
						are_identical = false;
						break;
					}
					node1 = node1->getParent();
					node2 = node2->getParent();
				}
				
				assert (!are_identical);
			}
		}
	}

	sanityCheck();
#endif
}

const std::list<ReachableFact*>& ReachableTree::getFactsAtLevel(unsigned int level) const
{
#ifdef MYPOP_SAS_PLUS_REACHABLE_TREE_DEBUG
	sanityCheck();
#endif
	assert (reachable_set_->getReachableSets().size() > level);
	return *reachable_set_->getReachableSets()[level];
}

unsigned int ReachableTree::getMaxDepth() const
{
//	return reachable_set_->getFactsSet().size();
	return reachable_set_->getFactSet().getFacts().size();
}

void ReachableTree::addNewLeaf(ReachableTreeNode& new_leaf)
{
	assert (new_leaf.getLevel() == getMaxDepth() - 1);
	leaf_nodes_.push_back(&new_leaf);
	
#ifdef MYPOP_SAS_PLUS_REACHABLE_TREE_DEBUG
	std::cout << this << "[ReachableTree] New leaf node:" << std::endl;
	ReachableTreeNode* leaf = &new_leaf;
	do
	{
		std::cout << "* " << *leaf << "(" << leaf << ")" << std::endl;
	} while ((leaf = leaf->getParent()) != NULL);
#endif
}

void ReachableTree::sanityCheck() const
{
	for (std::vector<ReachableTreeNode*>::const_iterator ci = leaf_nodes_.begin(); ci != leaf_nodes_.end(); ci++)
	{
		assert (*ci != NULL);
		assert (&(*ci)->getTree() != NULL);
		assert (&(*ci)->getTree() == this);
		(*ci)->sanityCheck();
	}
	
	assert (reachable_set_ != NULL);
}

std::ostream& operator<<(std::ostream& os, const ReachableTree& reachable_tree)
{
	os << "[ReachableTree - " << &reachable_tree << "^" << &reachable_tree.getReachableSet().getConstraints() << "] " << *reachable_tree.getRoot();
	return os;
}
    const ReachableTree* reachable_tree_;

};

};
