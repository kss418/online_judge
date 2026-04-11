export const submissionStatusOptions = [
  { value: '', label: '전체' },
  { value: 'accepted', label: '정답' },
  { value: 'wrong_answer', label: '오답' },
  { value: 'time_limit_exceeded', label: '시간 초과' },
  { value: 'memory_limit_exceeded', label: '메모리 초과' },
  { value: 'runtime_error', label: '런타임 에러' },
  { value: 'output_exceeded', label: '출력 초과' },
  { value: 'compile_error', label: '컴파일 에러' }
]

const validSubmissionStatusFilterValues = new Set(
  submissionStatusOptions
    .map((option) => option.value)
    .filter(Boolean)
)

export function normalizeProblemIdFilterInputValue(value){
  if (value === null || typeof value === 'undefined') {
    return ''
  }

  return String(value).trim()
}

export function normalizeUserLoginIdFilterInputValue(value){
  if (value === null || typeof value === 'undefined') {
    return ''
  }

  return String(value).trim()
}

function parsePositiveIntegerString(value){
  const parsedValue = Number.parseInt(value, 10)
  return Number.isInteger(parsedValue) && parsedValue > 0
    ? String(parsedValue)
    : ''
}

export function parseRouteQuery(query){
  const parsedProblemId = Number.parseInt(query.problemId, 10)

  return {
    scope: query.scope === 'mine' ? 'mine' : '',
    problemIdFilter: Number.isInteger(parsedProblemId) && parsedProblemId > 0
      ? String(parsedProblemId)
      : '',
    userLoginId: typeof query.userLoginId === 'string'
      ? query.userLoginId.trim()
      : '',
    status: validSubmissionStatusFilterValues.has(query.status)
      ? query.status
      : '',
    language: typeof query.language === 'string'
      ? query.language.trim()
      : ''
  }
}

export function buildRouteQuery(state, options = {}){
  const {
    hasFixedProblemId = false,
    isMineScope = false,
    routeName = ''
  } = options
  const nextQuery = {}

  if (
    routeName !== 'problem-my-submissions' &&
    routeName !== 'problem-submissions' &&
    state.scope === 'mine'
  ) {
    nextQuery.scope = 'mine'
  }

  if (!hasFixedProblemId && state.problemIdFilter) {
    nextQuery.problemId = parsePositiveIntegerString(state.problemIdFilter)
  }

  if (!isMineScope && state.userLoginId) {
    nextQuery.userLoginId = String(state.userLoginId).trim()
  }

  if (state.status) {
    nextQuery.status = state.status
  }

  if (state.language) {
    nextQuery.language = String(state.language).trim()
  }

  return nextQuery
}

export function buildApiQuery(state, options = {}){
  const {
    fixedProblemId = null,
    isMineScope = false,
    currentUserId = null,
    beforeSubmissionId = null,
    limit = null
  } = options
  const normalizedProblemId = fixedProblemId != null
    ? fixedProblemId
    : Number.parseInt(state.problemIdFilter, 10)

  return {
    before_submission_id: Number.isInteger(beforeSubmissionId) && beforeSubmissionId > 0
      ? beforeSubmissionId
      : null,
    limit: Number.isInteger(limit) && limit > 0 ? limit : null,
    problem_id: Number.isInteger(normalizedProblemId) && normalizedProblemId > 0
      ? normalizedProblemId
      : null,
    user_id: isMineScope && Number.isInteger(currentUserId) && currentUserId > 0
      ? currentUserId
      : null,
    user_login_id: !isMineScope && state.userLoginId
      ? String(state.userLoginId).trim()
      : '',
    status: state.status,
    language: state.language
  }
}
