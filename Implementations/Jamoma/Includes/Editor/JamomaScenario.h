/*!
 * \file JamomaScenario.h
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

#include "Editor/Message.h"
#include "Editor/Scenario.h"
#include "Editor/State.h"
#include "Editor/TimeConstraint.h"
#include "Editor/TimeEvent.h"
#include "Editor/TimeNode.h"
#include "Editor/TimeProcess.h"
#include "Editor/TimeValue.h"

#include "JamomaClock.h"
#include "JamomaScenario.h"
#include "JamomaTimeConstraint.h"
#include "JamomaTimeEvent.h"
#include "JamomaTimeProcess.h"
#include "JamomaTimeNode.h"

using namespace OSSIA;
using namespace std;

class JamomaScenario final : public Scenario, public JamomaTimeProcess
{

private:

# pragma mark -
# pragma mark Implementation specific

  Container<TimeConstraint>   mTimeContraints;
  Container<TimeNode>         mTimeNodes;         // list of all TimeNodes of the scenario (the first is the start node, the second is the end node)

  shared_ptr<State>           mCurrentState;      // an internal State to return on state call

public:

# pragma mark -
# pragma mark Life cycle

  JamomaScenario(shared_ptr<State>,
                 shared_ptr<State>);

  JamomaScenario(const JamomaScenario *);

  shared_ptr<Scenario> clone() const override;

  ~JamomaScenario();

# pragma mark -
# pragma mark Execution

  shared_ptr<StateElement> state(const TimeValue&, const TimeValue&) override;

# pragma mark -
# pragma mark Execution - Implementation specific

  void start() override;
  void stop() override;
  void pause() override;
  void resume() override;

# pragma mark -
# pragma mark Edition

  void addTimeConstraint(const shared_ptr<TimeConstraint>) override;

  void removeTimeConstraint(const shared_ptr<TimeConstraint>) override;

  void addTimeNode(const shared_ptr<TimeNode>) override;

  void removeTimeNode(const shared_ptr<TimeNode>) override;

# pragma mark -
# pragma mark Accessors

  const shared_ptr<TimeNode> & getStartTimeNode() const override;

  const shared_ptr<TimeNode> & getEndTimeNode() const override;

# pragma mark -
# pragma mark TimeNodes and TimeConstraints

  const Container<TimeNode>& timeNodes() const override;

  const Container<TimeConstraint>& timeConstraints() const override;

private:

# pragma mark -
# pragma mark Implementation specific

  /*! append each message of the state to the current state in order to eliminate address redundancy
  \todo this code is also in JamomaLoop so we should find a way to factorise it */
  void flattenAndFilter(const shared_ptr<StateElement>);
};
