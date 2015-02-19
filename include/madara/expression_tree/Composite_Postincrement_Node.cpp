/* -*- C++ -*- */
#ifndef _COMPOSITE_PREINCREMENT_NODE_CPP_
#define _COMPOSITE_PREINCREMENT_NODE_CPP_

#ifndef _MADARA_NO_KARL_

#include <iostream>

#include "madara/expression_tree/Component_Node.h"
#include "madara/expression_tree/Composite_Unary_Node.h"
#include "madara/expression_tree/Visitor.h"
#include "madara/expression_tree/Composite_Postincrement_Node.h"
#include "madara/expression_tree/Leaf_Node.h"
#include "madara/expression_tree/Variable_Node.h"

#include "madara/utility/Log_Macros.h"

// Ctor

Madara::Expression_Tree::Composite_Postincrement_Node::Composite_Postincrement_Node (
  Component_Node *right)
  : Composite_Unary_Node (right)
{
  var_ = dynamic_cast <Variable_Node *> (right);

  if (!var_)
    array_ = dynamic_cast <Composite_Array_Reference *> (right);
}

// Dtor
Madara::Expression_Tree::Composite_Postincrement_Node::~Composite_Postincrement_Node (void)
{
}

Madara::Knowledge_Record
Madara::Expression_Tree::Composite_Postincrement_Node::item (void) const
{
  Madara::Knowledge_Record record;
  record.set_value ("++");
  return record;
}

/// Prune the tree of unnecessary nodes. 
/// Returns evaluation of the node and sets can_change appropriately.
/// if this node can be changed, that means it shouldn't be pruned.
Madara::Knowledge_Record
Madara::Expression_Tree::Composite_Postincrement_Node::prune (bool & can_change)
{
  bool right_child_can_change = false;
  Madara::Knowledge_Record right_value;

  if (this->right_)
  {
    right_value = this->right_->prune (right_child_can_change);
    if (!right_child_can_change && dynamic_cast <Leaf_Node *> (right_) == 0)
    {
      delete this->right_;
      this->right_ = new Leaf_Node (right_value);
    }
  }
  else
  {
    MADARA_ERROR (MADARA_LOG_TERMINAL_ERROR, (LM_ERROR, DLINFO
      "\nKARL COMPILE ERROR: Preincrement" \
      " has no right expression\n"));
    exit (-1);
  }

  can_change = right_child_can_change;

  return ++right_value;
}

/// Evaluates the node and its children. This does not prune any of
/// the expression tree, and is much faster than the prune function
Madara::Knowledge_Record 
Madara::Expression_Tree::Composite_Postincrement_Node::evaluate (
  const Madara::Knowledge_Engine::Knowledge_Update_Settings & settings)
{
  Madara::Knowledge_Record return_value;

  if (var_)
  {
    return_value = var_->evaluate (settings);
    var_->inc (settings);
  }
  else if (array_)
  {
    return_value = array_->evaluate (settings);
    array_->inc (settings);
  }
  else
  {
    return_value = ++(this->right_->evaluate (settings));
  }

  return return_value;
}

// accept a visitor
void 
Madara::Expression_Tree::Composite_Postincrement_Node::accept (Visitor &visitor) const
{
  visitor.visit (*this);
}

#endif // _MADARA_NO_KARL_

#endif /* _COMPOSITE_PREINCREMENT_NODE_CPP_ */
