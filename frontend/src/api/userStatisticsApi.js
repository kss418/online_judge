import { requestJson } from '@/api/http'
import {
  normalizeSolvedProblemsResponse,
  normalizeUserSubmissionStatisticsResponse,
  normalizeWrongProblemsResponse
} from '@/api/userResponse'

export function getUserSubmissionStatistics(userId){
  return requestJson(`/user/${userId}/statistics`)
    .then(normalizeUserSubmissionStatisticsResponse)
}

export function getUserSolvedProblems(userId, bearerToken){
  return requestJson(`/user/${userId}/solved-problems`, {
    bearerToken
  }).then(normalizeSolvedProblemsResponse)
}

export function getUserWrongProblems(userId, bearerToken){
  return requestJson(`/user/${userId}/wrong-problems`, {
    bearerToken
  }).then(normalizeWrongProblemsResponse)
}
