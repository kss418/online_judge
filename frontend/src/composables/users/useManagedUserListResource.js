import { computed } from 'vue'

import { getUserList, getUserSubmissionBan } from '@/api/userAdminApi'
import { useAsyncResource } from '@/composables/useAsyncResource'
import {
  createUserListErrorMessage,
  mergeManagedUserBanStatuses,
  patchUserInList
} from '@/composables/users/userListResourceShared'
import { noticeStore } from '@/stores/notice/noticeStore'
import { createSubmissionBanState } from '@/utils/submissionBan'

function createBaseManagedUser(user){
  return {
    ...user,
    ...createSubmissionBanState(),
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

async function loadManagedUsers(token){
  const response = await getUserList(token)
  const users = Array.isArray(response.users)
    ? response.users.map(createBaseManagedUser)
    : []

  if (!users.length) {
    return createInitialUserManagementState()
  }

  const statusResults = await Promise.all(
    users.map(async (user) => {
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
          ...createSubmissionBanState(),
          has_error: true
        }
      }
    })
  )

  return mergeManagedUserBanStatuses(users, statusResults)
}

export function useManagedUserListResource({
  authState,
  canLoad
}){
  const userManagementResource = useAsyncResource({
    initialData: createInitialUserManagementState,
    load: loadManagedUsers,
    getErrorMessage: createUserListErrorMessage
  })
  const users = computed(() => userManagementResource.data.value.users)

  function resetUsers(){
    userManagementResource.reset({
      preserveHasLoadedOnce: true,
      clearLastArgs: true
    })
  }

  async function loadUsers(){
    if (!authState?.token || !canLoad?.value) {
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
      users: patchUserInList(currentState.users, userId, patch)
    }))
  }

  return {
    users,
    isLoading: userManagementResource.isLoading,
    errorMessage: userManagementResource.errorMessage,
    hasLoadedOnce: userManagementResource.hasLoadedOnce,
    resetUsers,
    loadUsers,
    patchUser
  }
}
