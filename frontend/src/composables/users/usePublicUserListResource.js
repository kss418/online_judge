import { getPublicUserList } from '@/api/userQueryApi'
import { useAsyncResource } from '@/composables/useAsyncResource'
import { createUserListErrorMessage } from '@/composables/users/userListResourceShared'

function createInitialUserList(){
  return []
}

export function usePublicUserListResource(){
  const userListResource = useAsyncResource({
    initialData: createInitialUserList,
    async load(query){
      const response = await getPublicUserList(query)
      return Array.isArray(response.users) ? response.users : []
    },
    getErrorMessage: createUserListErrorMessage
  })

  async function loadUsers(query = ''){
    return userListResource.run(query, {
      resetDataOnError: true
    })
  }

  return {
    users: userListResource.data,
    isLoading: userListResource.isLoading,
    errorMessage: userListResource.errorMessage,
    hasLoadedOnce: userListResource.hasLoadedOnce,
    loadUsers
  }
}
