/*!
 * \file JamomaTimeNode.h
 *
 * \brief
 *
 * \details
 *
 * \author Théo de la Hogue
 *
 * \copyright This code is licensed under the terms of the "CeCILL-C"
 * http://www.cecill.info
 */

#pragma once

#include "Editor/TimeConstraint.h"
#include "Editor/TimeNode.h"
#include "Editor/TimeValue.h"

#include "JamomaTimeEvent.h" // because the TimeNode::emplace method is a JamomaTimeEvent factory

using namespace OSSIA;
using namespace std;

class JamomaTimeNode : public TimeNode, public enable_shared_from_this<JamomaTimeNode>
{

private:

# pragma mark -
# pragma mark Implementation specific
  
  shared_ptr<Expression>    mExpression;
  bool                      mObserveExpression;
  Expression::iterator      mResultCallbackIndex;
  
  TimeValue                 mSimultaneityMargin;
  
public:

# pragma mark -
# pragma mark Life cycle
  
  JamomaTimeNode();
  
  JamomaTimeNode(const JamomaTimeNode *);
  
  shared_ptr<TimeNode> clone() const override;
  
  ~JamomaTimeNode();

# pragma mark -
# pragma mark Execution
  
  void play(bool log = false, string name = "") const override;

# pragma mark -
# pragma mark Accessors
  
  TimeValue getDate() const override;
  
  const std::shared_ptr<Expression> & getExpression() const override;
  
  TimeNode & setExpression(const std::shared_ptr<Expression>) override;
  
  TimeValue getSimultaneityMargin() const override;
  
  TimeNode & setSimultaneityMargin(const TimeValue&) override;
  
# pragma mark -
# pragma mark #TimeEvents
  
  iterator emplace(const_iterator,
                   TimeEvent::ExecutionCallback,
                   shared_ptr<Expression> = nullptr) override;
  
# pragma mark -
# pragma mark Implementation specific
  
  /* process all TimeEvents to propagate execution */
  void process(Container<TimeEvent>& statusChangedEvents);
  
  /* is the TimeNode observing its Expression ? */
  bool isObservingExpression();
  
private:
  
  void observeExpressionResult(bool);

  void resultCallback(bool result);
};
