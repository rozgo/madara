
#ifndef _MADARA_NO_KARL_

#include "madara/utility/Log_Macros.h"
#include "madara/expression_tree/Leaf_Node.h"
#include "madara/expression_tree/System_Call_Type.h"
#include "madara/expression_tree/Visitor.h"


Madara::Expression_Tree::System_Call_Type::System_Call_Type (
  Madara::Knowledge_Engine::Thread_Safe_Context & context,
  const Component_Nodes & nodes)
  : System_Call_Node (context, nodes)
{

}

// Dtor
Madara::Expression_Tree::System_Call_Type::~System_Call_Type (void)
{
}

Madara::Knowledge_Record
Madara::Expression_Tree::System_Call_Type::item (void) const
{
  return Madara::Knowledge_Record::Integer (nodes_.size ());
}

/// Prune the tree of unnecessary nodes. 
/// Returns evaluation of the node and sets can_change appropriately.
/// if this node can be changed, that means it shouldn't be pruned.
Madara::Knowledge_Record
Madara::Expression_Tree::System_Call_Type::prune (bool & can_change)
{
  // user can always change a function, and we have no control over
  // what it does. Consequently, a function node cannot be pruned out
  // under any situation
  can_change = true;
  
  Madara::Knowledge_Record result;

  if (nodes_.size () > 0)
  {
    bool arg_can_change = false;
    result = nodes_[0]->prune (arg_can_change);
    
    if (!arg_can_change && dynamic_cast <Leaf_Node *> (nodes_[0]) == 0)
    {
      delete nodes_[0];
      nodes_[0] = new Leaf_Node (result);
    }
  }
  else
  {
    MADARA_DEBUG (MADARA_LOG_EMERGENCY, (LM_ERROR, 
      "KARL COMPILE ERROR: System call type requires an argument.\n"));
  }

  return result;
}

/// Evaluates the node and its children. This does not prune any of
/// the expression tree, and is much faster than the prune function
Madara::Knowledge_Record 
Madara::Expression_Tree::System_Call_Type::evaluate (
const Madara::Knowledge_Engine::Knowledge_Update_Settings & settings)
{
  Knowledge_Record return_value;

  if (nodes_.size () > 0)
  {
    MADARA_DEBUG (MADARA_LOG_MINOR_EVENT, (LM_DEBUG, 
      "System call type is returning the type of its first argument.\n"));

    return Madara::Knowledge_Record::Integer (
      nodes_[0]->evaluate (settings).type ());
  }
  else
  {
    MADARA_DEBUG (MADARA_LOG_EMERGENCY, (LM_DEBUG, 
      "KARL RUNTIME ERROR: System call type requires an argument.\n"));
  }

  return return_value;
}

// accept a visitor
void 
Madara::Expression_Tree::System_Call_Type::accept (
  Madara::Expression_Tree::Visitor &visitor) const
{
  visitor.visit (*this);
}

#endif // _MADARA_NO_KARL_
