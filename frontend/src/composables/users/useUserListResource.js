import { computed } from 'vue'

import { getUserList, getUserSubmissionBan } from '@/api/userAdminApi'
import { getPublicUserList } from '@/api/userQueryApi'
import { useAsyncResource } from '@/composables/useAsyncResource'
import { noticeStore } from '@/stores/notice/noticeStore'
import { formatApiError } from '@/utils/apiError'

function createInitialUserList(){
  return []
}

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

function createUserListErrorMessage(error){
  return formatApiError(error, {
    fallback: '유저 목록을 불러오지 못했습니다.'
  })
}

async function loadManagedUsers(token){
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
}

export function useUserListResource({
  mode,
  authState,
  canLoad
}){
  if (mode === 'public') {
    const userListResource = useAsyncResource({
      initialData: createInitialUserList,
      async load(query){
        const response = await getPublicUserList(query)
        return Array.isArray(response.users) ? response.users : []
      },
      getErrorMessage: createUserListErrorMessage
    })

    function resetUsers(){
      userListResource.reset({
        clearLastArgs: true
      })
    }

    async function loadUsers(query = ''){
      return userListResource.run(query, {
        resetDataOnError: true
      })
    }

    function patchUser(userId, patch){
      userListResource.mutate((users) =>
        users.map((user) =>
          user.user_id === userId
            ? {
              ...user,
              ...patch
            }
            : user
        )
      )
    }

    return {
      users: userListResource.data,
      isLoading: userListResource.isLoading,
      errorMessage: userListResource.errorMessage,
      hasLoadedOnce: userListResource.hasLoadedOnce,
      resetUsers,
      loadUsers,
      patchUser
    }
  }

  if (mode === 'admin') {
    const userListResource = useAsyncResource({
      initialData: createInitialUserList,
      async load(token){
        const response = await getUserList(token)
        return Array.isArray(response.users) ? response.users : []
      },
      getErrorMessage: createUserListErrorMessage
    })

    function resetUsers(){
      userListResource.reset({
        preserveHasLoadedOnce: true,
        clearLastArgs: true
      })
    }

    async function loadUsers(){
      if (!authState?.token || !canLoad?.value) {
        resetUsers()

        return {
          status: 'reset',
          data: userListResource.data.value
        }
      }

      return userListResource.run(authState.token, {
        resetDataOnError: true
      })
    }

    function patchUser(userId, patch){
      userListResource.mutate((users) =>
        users.map((user) =>
          user.user_id === userId
            ? {
              ...user,
              ...patch
            }
            : user
        )
      )
    }

    return {
      users: userListResource.data,
      isLoading: userListResource.isLoading,
      errorMessage: userListResource.errorMessage,
      hasLoadedOnce: userListResource.hasLoadedOnce,
      resetUsers,
      loadUsers,
      patchUser
    }
  }

  if (mode === 'management') {
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
      users,
      isLoading: userManagementResource.isLoading,
      errorMessage: userManagementResource.errorMessage,
      hasLoadedOnce: userManagementResource.hasLoadedOnce,
      resetUsers,
      loadUsers,
      patchUser
    }
  }

  throw new Error(`Unsupported user list resource mode: ${mode}`)
}
