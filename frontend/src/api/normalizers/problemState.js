export function normalizeProblemState(problemState){
  return problemState === 'solved' || problemState === 'wrong'
    ? problemState
    : null
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
