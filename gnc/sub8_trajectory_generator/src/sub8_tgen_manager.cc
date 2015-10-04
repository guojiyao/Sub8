/**
 * Author: Patrick Emami
 * Date: 9/29/15
 *
 */

#include "sub8_tgen_manager.h"
#include "sub8_tgen_common.h"
#include "ompl/base/PlannerStatus.h"
#include "ompl/base/goals/GoalState.h"
#include "ompl/control/planners/pdst/PDST.h"
#include "ompl/control/planners/rrt/RRT.h"
#include "ompl/control/PathControl.h"

#include <ros/console.h>

using sub8::trajectory_generator::Sub8TGenManager;
using sub8::trajectory_generator::Sub8TGenMsgs;
using sub8::trajectory_generator::Sub8SpaceInformationGeneratorPtr;
using sub8::trajectory_generator::Sub8StateSpace;
using ompl::base::Planner;
using ompl::base::PlannerStatus;
using ompl::base::GoalState;
using ompl::base::ProblemDefinitionPtr;
using ompl::control::RRT;
using ompl::control::PDST;
using ompl::control::PathControl;

Sub8TGenManager::Sub8TGenManager(int planner) {
  Sub8SpaceInformationGeneratorPtr ss_gen(new Sub8SpaceInformationGenerator());
  _sub8_si = ss_gen->generate();

  // Instantiate the planner indicated in the launch file
  // Switch on the PlannerType
  switch (planner) {
    case PlannerType::PDST:
      _sub8_planner =
          boost::shared_ptr<Planner>(new ompl::control::PDST(_sub8_si));
      break;
    default:
      _sub8_planner =
          boost::shared_ptr<Planner>(new ompl::control::RRT(_sub8_si));
      break;
  }
}

void Sub8TGenManager::setProblemDefinition(const State* start_state,
                                           const State* goal_state) {
  if (_sub8_planner->isSetup()) {
    _sub8_planner->clear();
    ProblemDefinitionPtr pdef(new ProblemDefinition(_sub8_si));

    // set the start state for the new ProblemDefinition
    pdef->addStartState(start_state);
    // set the goal state for the new ProblemDefinition
    pdef->setGoalState(goal_state);

    _sub8_planner->setProblemDefinition(pdef);

    // Verifies that the problem definition was correctly set
    _sub8_planner->checkValidity();
  }
}

bool Sub8TGenManager::solve() {
  bool on_success = false;

  // Need to supply a real terminating condition
  PlannerStatus pstatus = _sub8_planner->solve(1.0);

  // Switch on the value of the PlannerStatus enum "StateType"
  switch (pstatus.operator StatusType()) {
    case PlannerStatus::INVALID_START:
      ROS_ERROR("%s", Sub8TGenMsgs::INVALID_START);
      // TODO - ALARM
      break;
    case PlannerStatus::INVALID_GOAL:
      ROS_ERROR("%s", Sub8TGenMsgs::INVALID_GOAL);
      // TODO - ALARM
      break;
    case PlannerStatus::UNRECOGNIZED_GOAL_TYPE:
      ROS_ERROR("%s", Sub8TGenMsgs::UNRECOGNIZED_GOAL_TYPE);
      // TODO - ALARM
      break;
    case PlannerStatus::TIMEOUT:
      ROS_ERROR("%s", Sub8TGenMsgs::TIMEOUT);
      // The logical flow here should be:
      //
      // 1. Check any safety paths?
      // 2. If no, set off ALARM
      // 3. If yes, send off safety path to controller, and then attempt to
      // replan
      //
      // Q: How many times do I retry replanning if failure?
      break;
    case PlannerStatus::APPROXIMATE_SOLUTION:
      ROS_DEBUG("%s", Sub8TGenMsgs::APPROXIMATE_SOLUTION);
      on_success = true;
      break;
    case PlannerStatus::EXACT_SOLUTION:
      ROS_DEBUG("%s", Sub8TGenMsgs::EXACT_SOLUTION);
      on_success = true;
      break;
    case PlannerStatus::CRASH:
      ROS_ERROR("%s", Sub8TGenMsgs::CRASH);
      // ALARM
      break;
    default:
      break;
  }

  return on_success;
}

void Sub8TGenManager::validateCurrentTrajectory() {
  // Planner get start state, get goal state
  unsigned int first_invalid_state = 0;

  ProblemDefinitionPtr pdef = _sub8_planner->getProblemDefinition();
  PathControl* spath = static_cast<PathControl*>(pdef->getSolutionPath().get());

  if (!_sub8_si->checkMotion(spath->getStates(), spath->getStateCount(),
                             first_invalid_state)) {
    ROS_WARN("%s", Sub8TGenMsgs::REPLAN_FAILED);
    // Go ahead and supply the controller with a safety path

    // naive replanning; will implement smarter re-planning later
    State* new_start_state = spath->getState(first_invalid_state);

    // Replan from the first invalid state to the goal state
    setProblemDefinition(
        new_start_state,
        static_cast<GoalState*>(pdef->getGoal().get())->getState());
    solve();

  } else {
    ROS_INFO("%s", Sub8TGenMsgs::TRAJECTORY_VALIDATED);
  }
}

State* Sub8TGenManager::waypointToState(
    const boost::shared_ptr<sub8_msgs::Waypoint>& wpoint) {
  State* state = _sub8_si->getStateSpace()->allocState();

  state->as<Sub8StateSpace::StateType>()->setX(wpoint->pos.position.x);
  state->as<Sub8StateSpace::StateType>()->setY(wpoint->pos.position.y);
  state->as<Sub8StateSpace::StateType>()->setZ(wpoint->pos.position.z);
  state->as<Sub8StateSpace::StateType>()->setXDot(wpoint->vel.linear.x);
  state->as<Sub8StateSpace::StateType>()->setYDot(wpoint->vel.linear.y);
  state->as<Sub8StateSpace::StateType>()->setZDot(wpoint->vel.linear.z);
  state->as<Sub8StateSpace::StateType>()->setWx(wpoint->vel.angular.x);
  state->as<Sub8StateSpace::StateType>()->setWy(wpoint->vel.angular.y);
  state->as<Sub8StateSpace::StateType>()->setWz(wpoint->vel.angular.z);
  state->as<Sub8StateSpace::StateType>()->setQx(wpoint->pos.orientation.x);
  state->as<Sub8StateSpace::StateType>()->setQy(wpoint->pos.orientation.y);
  state->as<Sub8StateSpace::StateType>()->setQz(wpoint->pos.orientation.z);
  state->as<Sub8StateSpace::StateType>()->setQw(wpoint->pos.orientation.w);
  return state;
}