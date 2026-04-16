import { formatApiError } from '@/utils/apiError'
import { createSubmissionBanState } from '@/utils/submissionBan'

export function createUserListErrorMessage(error){
  return formatApiError(error, {
    fallback: '유저 목록을 불러오지 못했습니다.'
  })
}

export function patchUserInList(users, userId, patch){
  return users.map((user) =>
    user.user_id === userId
      ? {
        ...user,
        ...patch
      }
      : user
  )
}

export function mergeManagedUserBanStatuses(users, statusResults){
  const statusResultMap = new Map(
    statusResults.map((statusResult) => [statusResult.user_id, statusResult])
  )
  const failedBanStatusCount = statusResults.filter((statusResult) => statusResult.has_error).length

  return {
    users: users.map((user) => {
      const statusResult = statusResultMap.get(user.user_id)

      if (!statusResult) {
        return {
          ...user,
          ...createSubmissionBanState(),
          submission_ban_status_loading: false,
          submission_ban_status_error: true
        }
      }

      return {
        ...user,
        submission_banned_until: statusResult.submission_banned_until,
        submission_banned_until_timestamp: statusResult.submission_banned_until_timestamp,
        submission_banned_until_label: statusResult.submission_banned_until_label,
        submission_ban_status_loading: false,
        submission_ban_status_error: statusResult.has_error
      }
    }),
    failedBanStatusCount
  }
}
