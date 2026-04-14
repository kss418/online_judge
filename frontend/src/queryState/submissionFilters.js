import { submissionStatusOptions as generatedSubmissionStatusOptions } from '@/generated/submissionStatusCatalog'
import { parsePositiveInteger } from '@/utils/parse'

export const submissionStatusOptions = generatedSubmissionStatusOptions

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

function normalizePositiveIntegerString(value){
  const parsedValue = parsePositiveInteger(value)

  return parsedValue != null ? String(parsedValue) : ''
}

export function parseRouteQuery(query){
  const problemIdQueryValue = Array.isArray(query.problemId)
    ? query.problemId[0]
    : query.problemId

  return {
    scope: query.scope === 'mine' ? 'mine' : '',
    problemIdFilter: normalizePositiveIntegerString(problemIdQueryValue),
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

  const normalizedProblemId = normalizePositiveIntegerString(state.problemIdFilter)

  if (!hasFixedProblemId && normalizedProblemId) {
    nextQuery.problemId = normalizedProblemId
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
    : parsePositiveInteger(state.problemIdFilter)

  return {
    before_submission_id: Number.isInteger(beforeSubmissionId) && beforeSubmissionId > 0
      ? beforeSubmissionId
      : null,
    limit: Number.isInteger(limit) && limit > 0 ? limit : null,
    problem_id: normalizedProblemId ?? null,
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
