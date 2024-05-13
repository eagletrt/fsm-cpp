#pragma once

#include <stdio.h>

#include <mutex>
#include <optional>

#include "errors/custom_exceptions.hpp"
#include "errors/errors.hpp"

template <unsigned int States, typename StatesEnumType, typename StateData,
          typename EventData = StatesEnumType>
class FSM {
  using stateFunctionT = StatesEnumType (*)(const FSM &, StateData &);
  using transitionFunctionT = void (*)(const FSM &, StateData &);

 public:
  FSM(StateData &_stateData, StatesEnumType initialState)
      : currentState(initialState), stateData(_stateData) {
    std::scoped_lock lck(mtx);
    for (unsigned int i = 0; i < States; i++) {
      stateFunctions[i] = nullptr;
      for (unsigned int j = 0; j < States; j++) {
        transitionFunctions[i][j] = nullptr;
        allowedTransitions[i][j] = false;
      }
    }
    event = std::nullopt;
  }

  StatesEnumType run() {
    std::scoped_lock lck(mtx);
    if (!stateFunctions[(int)currentState]) {
      throw err::exception_invalid_state_function((int)currentState);
    }
    auto requestedState = stateFunctions[(int)currentState](*this, stateData);
    if (allowedTransitions[(int)currentState][(int)requestedState]) {
      auto transitionFunction =
          transitionFunctions[(int)currentState][(int)requestedState];
      if (transitionFunction) {
        transitionFunction(*this, stateData);
      }
      currentState = requestedState;
    } else {
      throw err::exception_invalid_transition((int)currentState,
                                              (int)requestedState);
    }
    event = std::nullopt;
    return currentState;
  }

  void triggerEvent(const EventData &data) {
    std::scoped_lock lck(mtx);
    event = data;
  }
  std::optional<EventData> getEvent() const { return event; }

  void setStateFunction(StatesEnumType state, stateFunctionT stateFunction) {
    std::scoped_lock lck(mtx);
    stateFunctions[(int)state] = stateFunction;
  }
  void allowTransition(StatesEnumType from, StatesEnumType to,
                       transitionFunctionT transitionFunction) {
    std::scoped_lock lck(mtx);
    allowedTransitions[(int)from][(int)to] = true;
    transitionFunctions[(int)from][(int)to] = transitionFunction;
  }
  void allowTransition(StatesEnumType from, StatesEnumType to) {
    std::scoped_lock lck(mtx);
    allowedTransitions[(int)from][(int)to] = true;
  }
  StatesEnumType getState() const { return currentState; }

 private:
  transitionFunctionT transitionFunctions[States][States];
  bool allowedTransitions[States][States];
  stateFunctionT stateFunctions[States];
  StatesEnumType currentState;
  StateData &stateData;

  std::mutex mtx;
  std::optional<EventData> event;
};
