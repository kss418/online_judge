const problemStatePresentationMap = {
  solved: {
    label: '성공',
    tone: 'success',
    textClass: 'problem-state-text--solved'
  },
  wrong: {
    label: '실패',
    tone: 'danger',
    textClass: 'problem-state-text--wrong'
  }
}

export function normalizeProblemState(problemState){
  return problemState === 'solved' || problemState === 'wrong'
    ? problemState
    : null
}

export function getProblemStateLabel(problemState){
  return problemStatePresentationMap[normalizeProblemState(problemState)]?.label || ''
}

export function getProblemStateTone(problemState){
  return problemStatePresentationMap[normalizeProblemState(problemState)]?.tone || 'neutral'
}

export function getProblemStateTextClass(problemState){
  return problemStatePresentationMap[normalizeProblemState(problemState)]?.textClass || ''
}

export function normalizeProblemStateRecord(record){
  if (!record || typeof record !== 'object') {
    return record
  }

  return {
    ...record,
    user_problem_state: normalizeProblemState(record.user_problem_state)
  }
}

export function normalizeProblemStateRecords(records){
  return Array.isArray(records)
    ? records.map(normalizeProblemStateRecord)
    : []
}
