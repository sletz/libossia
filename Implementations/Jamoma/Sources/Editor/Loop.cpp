#include "Editor/JamomaLoop.h"

#include <iostream> //! \todo to remove. only here for debug purpose

# pragma mark -
# pragma mark Life cycle

namespace OSSIA
{
  shared_ptr<Loop> Loop::create()
  {
    return make_shared<JamomaLoop>();
  }
}

JamomaLoop::JamomaLoop() :
JamomaTimeProcess(State::create(), State::create())
{
  mPatternNode = TimeNode::create();
  mPatternNode->emplace(mPatternNode->timeEvents().begin(), std::bind(&JamomaLoop::PatternStartEventCallback, this, _1));
  mPatternNode->emplace(mPatternNode->timeEvents().end(), std::bind(&JamomaLoop::PatternEndEventCallback, this, _1));
  
  mPatternConstraint = TimeConstraint::create(std::bind(&JamomaLoop::PatternConstraintCallback, this, _1, _2, _3),
                                              mPatternNode->timeEvents()[0],
                                              mPatternNode->timeEvents()[1]);
  
  // set pattern TimeConstraint's clock in external mode
  shared_ptr<JamomaClock> clock = dynamic_pointer_cast<JamomaClock>(mPatternConstraint);
  clock->setDriveMode(Clock::DriveMode::EXTERNAL);
  
  mCurrentState = State::create();
}

JamomaLoop::JamomaLoop(const JamomaLoop * other) :
JamomaTimeProcess(other->mStartState, other->mEndState),
mPatternConstraint(other->mPatternConstraint->clone()),
mPatternNode(other->mPatternNode->clone())
{}

shared_ptr<Loop> JamomaLoop::clone() const
{
  return make_shared<JamomaLoop>(this);
}

JamomaLoop::~JamomaLoop()
{}

# pragma mark -
# pragma mark Execution

shared_ptr<StateElement> JamomaLoop::state(const TimeValue& position, const TimeValue& date)
{
  // reset internal State
  mCurrentState->stateElements().clear();
  
  // if the time goes backward : initialize pattern TimeEvent's status and pattern TimeConstraint's clock
  if (position < mLastPosition)
  {
    TimeValue offset = std::fmod((double)date, (double)mPatternConstraint->getDuration());
    
    // be sure the clock is stopped
    mPatternConstraint->stop();
    
    //! \note maybe we should initialized TimeEvents with an Expression returning false to DISPOSED status ?
    
    shared_ptr<JamomaTimeEvent> start = dynamic_pointer_cast<JamomaTimeEvent>(mPatternNode->timeEvents()[0]);
    shared_ptr<JamomaTimeEvent> end = dynamic_pointer_cast<JamomaTimeEvent>(mPatternNode->timeEvents()[1]);
    
    if (offset == Zero)
    {
      start->setStatus(TimeEvent::Status::NONE);
      end->setStatus(TimeEvent::Status::NONE);
    }
    else
    {
      start->setStatus(TimeEvent::Status::HAPPENED);
      end->setStatus(TimeEvent::Status::NONE);
      
      flattenAndFilter(start->getState());
      
      // set end TimeEvent status depending on duration min and max
      if (date > mPatternConstraint->getDurationMin() &&
          date <= mPatternConstraint->getDurationMax())
      {
        end->setStatus(TimeEvent::Status::PENDING);
      }
      
      // set clock offset
      mPatternConstraint->setOffset(offset);
      
      // launch the clock
      mPatternConstraint->start();
    }
  }
  
  // if position hasn't been processed already
  if (position != mLastPosition)
  {
    mLastPosition = position;
    
    shared_ptr<JamomaTimeNode> node = dynamic_pointer_cast<JamomaTimeNode>(mPatternNode);
    shared_ptr<JamomaTimeEvent> start = dynamic_pointer_cast<JamomaTimeEvent>(mPatternNode->timeEvents()[0]);
    shared_ptr<JamomaTimeEvent> end = dynamic_pointer_cast<JamomaTimeEvent>(mPatternNode->timeEvents()[1]);
    
    // check if the loop can start
    switch(start->getStatus())
    {
        // NONE start TimeEvent becomes PENDING
      case TimeEvent::Status::NONE:
      {
        start->setStatus(TimeEvent::Status::PENDING);
        start->observeExpressionResult(true);
        
        end->setStatus(TimeEvent::Status::NONE);
        end->observeExpressionResult(false);
        
        break;
      }
        
        // PENDING start TimeEvent is ready for evaluation
      case TimeEvent::Status::PENDING:
      {
        if (start->getExpression()->evaluate())
        {
          start->happen();
          flattenAndFilter(start->getState());
        }
        else
          start->setStatus(TimeEvent::Status::DISPOSED);
        
        start->observeExpressionResult(false);
        
        break;
      }
        
        // HAPPENED start TimeEvent  propagates the execution to the end of the loop
      case TimeEvent::Status::HAPPENED:
      {
        // check if the loop can end
        switch(end->getStatus())
        {
            // NONE end TimeEvent becomes PENDING
          case TimeEvent::Status::NONE:
          {
            // when all pattern constraint durations are equals
            if (mPatternConstraint->getDurationMin() == mPatternConstraint->getDuration() &&
                mPatternConstraint->getDurationMax() == mPatternConstraint->getDuration())
            {
              end->observeExpressionResult(false);
              
              // stay NONE status
              break;
            }
            // when the minimal duration is not reached
            else if (mPatternConstraint->getDate() < mPatternConstraint->getDurationMin())
            {
              end->observeExpressionResult(false);
              
              // stay NONE status
              break;
            }
            // when the minimal duration is reached but not the maximal duration
            else if (mPatternConstraint->getDate() >= mPatternConstraint->getDurationMin() &&
                     mPatternConstraint->getDate() < mPatternConstraint->getDurationMax())
            {
              end->setStatus(TimeEvent::Status::PENDING);
              end->observeExpressionResult(true);
            }
            // when the maximal duration is reached
            else if (mPatternConstraint->getDate() >= mPatternConstraint->getDurationMax())
            {
              end->setStatus(TimeEvent::Status::PENDING);
              end->observeExpressionResult(false);
            }
            
            break;
          }
            
            // PENDING end TimeEvent is ready for evaluation
          case TimeEvent::Status::PENDING:
          {/*
            // observe and evaluate pattern TimeNode's expression before to go further
            if (node->getExpression() != ExpressionTrue && node->getExpression() != ExpressionFalse)
            {
              node->observeExpressionResult(true);
              if (!node->getExpression()->evaluate())
                break;
            }
            */
            if (end->getExpression()->evaluate())
            {
              end->happen();
              flattenAndFilter(end->getState());
            }
            else
              end->setStatus(TimeEvent::Status::DISPOSED);
            
            end->observeExpressionResult(false);
            node->observeExpressionResult(false);
            
            break;
          }
            
            // HAPPENED end TimeEvent propagates the execution to the start of the loop
          case TimeEvent::Status::HAPPENED:
          {
            start->setStatus(TimeEvent::Status::NONE);
            break;
          }
            
            // DISPOSED end TimeEvent stops the propagation of the execution
          case TimeEvent::Status::DISPOSED:
          {
            break;
          }
        }
        
        break;
      }
        
        // DISPOSED start TimeEvent stops the propagation of the execution
      case TimeEvent::Status::DISPOSED:
      {
        break;
      }
    }

    mPatternConstraint->tick();
  }

  //! \see JamomaLoop::PatternConstraintCallback below to understand how mCurrentState is filled
  return mCurrentState;
}

# pragma mark -
# pragma mark Accessors

const shared_ptr<TimeConstraint> JamomaLoop::getPatternTimeConstraint() const
{
  return mPatternConstraint;
}

const shared_ptr<TimeNode> JamomaLoop::getPatternTimeNode() const
{
  return mPatternNode;
}

# pragma mark -
# pragma mark Implementation specific

void JamomaLoop::flattenAndFilter(const shared_ptr<StateElement> element)
{
  switch (element->getType())
  {
    case StateElement::Type::MESSAGE :
    {
      shared_ptr<Message> messageToAppend = dynamic_pointer_cast<Message>(element);
      
      // find message with the same address to replace it
      bool found = false;
      for (auto it = mCurrentState->stateElements().begin();
           it != mCurrentState->stateElements().end();
           it++)
      {
        shared_ptr<Message> messageToCheck = dynamic_pointer_cast<Message>(*it);
        
        // replace if addresses are the same
        if (messageToCheck->getAddress() == messageToAppend->getAddress())
        {
          *it = element;
          found = true;
          break;
        }
      }
      
      // if not found append it
      if (!found)
        mCurrentState->stateElements().push_back(element);
      
      break;
    }
    case StateElement::Type::STATE :
    {
      shared_ptr<State> stateToFlatAndFilter = dynamic_pointer_cast<State>(element);
      
      for (const auto& e : stateToFlatAndFilter->stateElements())
      {
        flattenAndFilter(e);
      }
      break;
    }
  }
}

void JamomaLoop::PatternConstraintCallback(const TimeValue& position, const TimeValue& date, std::shared_ptr<StateElement> state)
{
  // add the state of the pattern TimeConstraint
  flattenAndFilter(mPatternConstraint->state(position, date));
}

void JamomaLoop::PatternStartEventCallback(TimeEvent::Status newStatus)
{
  //! \debug
  switch (newStatus)
  {
    case TimeEvent::Status::NONE:
    {
      cout << "Start Pattern Event NONE" << endl;
      break;
    }
    case TimeEvent::Status::PENDING:
    {
      cout << "Start Pattern Event PENDING" << endl;
      break;
    }
    case TimeEvent::Status::HAPPENED:
    {
      cout << "Start Pattern Event HAPPENED" << endl;
      break;
    }
    case TimeEvent::Status::DISPOSED:
    {
      cout << "Start Pattern Event DISPOSED" << endl;
      break;
    }
  }
}

void JamomaLoop::PatternEndEventCallback(TimeEvent::Status newStatus)
{
  //! \debug
  switch (newStatus)
  {
    case TimeEvent::Status::NONE:
    {
      cout << "End Pattern Event NONE" << endl;
      break;
    }
    case TimeEvent::Status::PENDING:
    {
      cout << "End Pattern Event PENDING" << endl;
      break;
    }
    case TimeEvent::Status::HAPPENED:
    {
      cout << "End Pattern Event HAPPENED" << endl;
      break;
    }
    case TimeEvent::Status::DISPOSED:
    {
      cout << "End Pattern Event DISPOSED" << endl;
      break;
    }
  }
}