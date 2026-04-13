import { computed } from 'vue'

import { getUserList, getUserSubmissionBan } from '@/api/userAdminApi'
import { useAsyncResource } from '@/composables/useAsyncResource'
import { noticeStore } from '@/stores/notice/noticeStore'
import { formatApiError } from '@/utils/apiError'

function createBaseManagedUser(user){
  return {
    ...user,
    submission_banned_until: null,
    submission_banned_until_timestamp: null,
    submission_banned_until_label: '',
    submission_ban_status_loading: true,
    submission_ban_status_error: false
  }
}

function createInitialUserManagementState(){
  return {
    users: [],
    failedBanStatusCount: 0
  }
}

export function useAdminUserManagementListResource({
  authState,
  canManageUsers
}){
  const userManagementResource = useAsyncResource({
    initialData: createInitialUserManagementState,
    async load(token){
      const response = await getUserList(token)
      const responseUsers = Array.isArray(response.users)
        ? response.users.map(createBaseManagedUser)
        : []

      if (!responseUsers.length) {
        return createInitialUserManagementState()
      }

      const statusResults = await Promise.all(
        responseUsers.map(async (user) => {
          try {
            const submissionBan = await getUserSubmissionBan(user.user_id, token)

            return {
              user_id: user.user_id,
              submission_banned_until: submissionBan.submission_banned_until,
              submission_banned_until_timestamp: submissionBan.submission_banned_until_timestamp,
              submission_banned_until_label: submissionBan.submission_banned_until_label,
              has_error: false
            }
          } catch {
            return {
              user_id: user.user_id,
              submission_banned_until: null,
              submission_banned_until_timestamp: null,
              submission_banned_until_label: '',
              has_error: true
            }
          }
        })
      )

      const statusResultMap = new Map(
        statusResults.map((statusResult) => [statusResult.user_id, statusResult])
      )
      const failedBanStatusCount = statusResults.filter((statusResult) => statusResult.has_error).length

      return {
        users: responseUsers.map((user) => {
          const statusResult = statusResultMap.get(user.user_id)

          if (!statusResult) {
            return {
              ...user,
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
    },
    getErrorMessage(error){
      return formatApiError(error, {
        fallback: '유저 목록을 불러오지 못했습니다.'
      })
    }
  })

  const users = computed(() => userManagementResource.data.value.users)

  function resetUsers(){
    userManagementResource.reset({
      preserveHasLoadedOnce: true,
      clearLastArgs: true
    })
  }

  async function loadUsers(){
    if (!authState.token || !canManageUsers.value) {
      resetUsers()

      return {
        status: 'reset',
        data: users.value
      }
    }

    const result = await userManagementResource.run(authState.token, {
      resetDataOnError: true
    })

    if (result.status === 'success' && result.data.failedBanStatusCount > 0) {
      noticeStore.showErrorNotice(
        `${result.data.failedBanStatusCount}명의 제출 밴 상태를 불러오지 못했습니다.`,
        { duration: 5000 }
      )
    }

    return result
  }

  function patchUser(userId, patch){
    userManagementResource.mutate((currentState) => ({
      ...currentState,
      users: currentState.users.map((user) =>
        user.user_id === userId
          ? {
            ...user,
            ...patch
          }
          : user
      )
    }))
  }

  return {
    isLoading: userManagementResource.isLoading,
    errorMessage: userManagementResource.errorMessage,
    users,
    resetUsers,
    loadUsers,
    patchUser
  }
}
