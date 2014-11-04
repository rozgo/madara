
#include "madara/utility/Log_Macros.h"
#include "madara/expression_tree/Leaf_Node.h"
#include "madara/expression_tree/System_Call_Expand_Statement.h"
#include "madara/knowledge_engine/Thread_Safe_Context.h"
#include "madara/expression_tree/Visitor.h"


Madara::Expression_Tree::System_Call_Expand_Statement::System_Call_Expand_Statement (
  Madara::Knowledge_Engine::Thread_Safe_Context & context,
  const Component_Nodes & nodes)
  : System_Call_Node (context, nodes)
{

}

// Dtor
Madara::Expression_Tree::System_Call_Expand_Statement::~System_Call_Expand_Statement (void)
{
}

Madara::Knowledge_Record
Madara::Expression_Tree::System_Call_Expand_Statement::item (void) const
{
  return Madara::Knowledge_Record::Integer (nodes_.size ());
}

/// Prune the tree of unnecessary nodes. 
/// Returns evaluation of the node and sets can_change appropriately.
/// if this node can be changed, that means it shouldn't be pruned.
Madara::Knowledge_Record
Madara::Expression_Tree::System_Call_Expand_Statement::prune (bool & can_change)
{
  // user can always change a function, and we have no control over
  // what it does. Consequently, a function node cannot be pruned out
  // under any situation
  can_change = true;
  
  Madara::Knowledge_Record result;

  if (nodes_.size () == 1)
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
      "KARL COMPILE ERROR: System call expand_statement requires 1 argument,"
      "e.g., #expand_statement ('var{.i}').\n"));
  }


  return result;
}

/// Evaluates the node and its children. This does not prune any of
/// the expression tree, and is much faster than the prune function
Madara::Knowledge_Record 
Madara::Expression_Tree::System_Call_Expand_Statement::evaluate (
const Madara::Knowledge_Engine::Knowledge_Update_Settings & settings)
{
  Knowledge_Record return_value;

  if (nodes_.size () == 1)
  {
    std::string statement (nodes_[0]->evaluate (settings).to_string ());

    MADARA_DEBUG (MADARA_LOG_MINOR_EVENT, (LM_DEBUG, 
      "System call expand_statement is returning the expansion "
      "of %s.\n", statement.c_str ()));

    return context_.expand_statement (statement);
  }
  else
  {
    MADARA_DEBUG (MADARA_LOG_EMERGENCY, (LM_ERROR, 
      "KARL COMPILE ERROR: System call expand_statement requires 1 argument,"
      "e.g., #expand_statement ('var{.i}').\n"));
  }

  return return_value;
}

// accept a visitor
void 
Madara::Expression_Tree::System_Call_Expand_Statement::accept (
  Madara::Expression_Tree::Visitor &visitor) const
{
  visitor.visit (*this);
}