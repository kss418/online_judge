import { computed } from 'vue'

import { getUserSummary } from '@/api/userQueryApi'
import { useAsyncResource } from '@/composables/useAsyncResource'
import { formatApiError } from '@/utils/apiError'

function createFallbackCurrentUser(){
  return {
    id: 0,
    user_login_id: '',
    role_name: 'user',
    permission_level: 0
  }
}

export function useMyProfileResource({
  route,
  authState,
  isAuthenticated
}){
  const currentUser = computed(() => authState.currentUser ?? createFallbackCurrentUser())
  const routeUserLoginId = computed(() => {
    const rawValue = Array.isArray(route.params.userLoginId)
      ? route.params.userLoginId[0]
      : route.params.userLoginId

    return typeof rawValue === 'string' ? rawValue.trim() : ''
  })
  const isUserProfileRoute = computed(() => route.name === 'user-info')
  const requestedProfileUserLoginId = computed(() => {
    if (isUserProfileRoute.value) {
      return routeUserLoginId.value
    }

    if (!authState.initialized || !isAuthenticated.value) {
      return ''
    }

    return currentUser.value.user_login_id ?? ''
  })
  const publicProfileResource = useAsyncResource({
    initialData: null,
    async load(userLoginId){
      return getUserSummary(userLoginId.trim())
    },
    getErrorMessage(error){
      return formatApiError(error, {
        fallback: '사용자 정보를 불러오지 못했습니다.'
      })
    }
  })

  const publicUserSummary = publicProfileResource.data
  const isPublicUserSummaryLoading = publicProfileResource.isLoading
  const publicUserSummaryErrorMessage = publicProfileResource.errorMessage
  const hasPublicProfile = computed(() => (
    Boolean(publicUserSummary.value) &&
    publicUserSummary.value.user_login_id === requestedProfileUserLoginId.value
  ))
  const activeProfileUserId = computed(() => {
    if (isUserProfileRoute.value) {
      return Number(publicUserSummary.value?.user_id ?? 0)
    }

    if (!authState.initialized || !isAuthenticated.value) {
      return 0
    }

    return Number(currentUser.value.id ?? 0)
  })
  const isOwnProfile = computed(() => (
    authState.initialized &&
    isAuthenticated.value &&
    (
      (
        isUserProfileRoute.value &&
        routeUserLoginId.value !== '' &&
        routeUserLoginId.value === (currentUser.value.user_login_id ?? '')
      ) ||
      (
        !isUserProfileRoute.value &&
        Number(currentUser.value.id ?? 0) > 0
      )
    )
  ))
  const showExtendedProfilePanels = computed(() => activeProfileUserId.value > 0)
  const shouldShowLoadingState = computed(() => (
    !isUserProfileRoute.value &&
    authState.isInitializing &&
    activeProfileUserId.value <= 0
  ))
  const displayedUser = computed(() => {
    if (hasPublicProfile.value) {
      return publicUserSummary.value
    }

    if (isOwnProfile.value) {
      return {
        user_id: Number(currentUser.value.id ?? 0),
        user_login_id: currentUser.value.user_login_id ?? '',
        created_at: null
      }
    }

    return {
      user_id: 0,
      user_login_id: routeUserLoginId.value,
      created_at: null
    }
  })
  const isProfileLoading = computed(() => {
    if (shouldShowLoadingState.value) {
      return true
    }

    if (!showExtendedProfilePanels.value) {
      return false
    }

    return isPublicUserSummaryLoading.value
  })
  const profileErrorMessage = computed(() => {
    if (isUserProfileRoute.value) {
      return publicUserSummaryErrorMessage.value
    }

    if (!showExtendedProfilePanels.value) {
      return ''
    }

    return publicUserSummaryErrorMessage.value
  })
  const profileStatusLabel = computed(() => {
    if (shouldShowLoadingState.value || isPublicUserSummaryLoading.value) {
      return 'Loading'
    }

    if (!showExtendedProfilePanels.value) {
      return 'Guest'
    }

    if (publicUserSummaryErrorMessage.value) {
      return 'Error'
    }

    return isOwnProfile.value ? 'Signed In' : 'Public'
  })
  const profileStatusTone = computed(() => {
    if (shouldShowLoadingState.value || isPublicUserSummaryLoading.value) {
      return 'neutral'
    }

    if (!showExtendedProfilePanels.value) {
      return 'neutral'
    }

    if (publicUserSummaryErrorMessage.value) {
      return 'danger'
    }

    return isOwnProfile.value ? 'success' : 'neutral'
  })

  function resetProfile(){
    publicProfileResource.reset({
      preserveHasLoadedOnce: true,
      clearLastArgs: true
    })
  }

  async function loadRequestedProfile(userLoginId = requestedProfileUserLoginId.value){
    const normalizedUserLoginId = typeof userLoginId === 'string'
      ? userLoginId.trim()
      : ''

    if (!normalizedUserLoginId) {
      resetProfile()

      if (isUserProfileRoute.value) {
        publicUserSummaryErrorMessage.value = '유효하지 않은 사용자입니다.'
      }

      return {
        status: 'error'
      }
    }

    return publicProfileResource.run(normalizedUserLoginId, {
      resetDataOnRun: true,
      resetDataOnError: true
    })
  }

  return {
    currentUser,
    routeUserLoginId,
    isUserProfileRoute,
    requestedProfileUserLoginId,
    publicUserSummary,
    isPublicUserSummaryLoading,
    publicUserSummaryErrorMessage,
    activeProfileUserId,
    isOwnProfile,
    showExtendedProfilePanels,
    shouldShowLoadingState,
    displayedUser,
    isProfileLoading,
    profileErrorMessage,
    profileStatusLabel,
    profileStatusTone,
    resetProfile,
    loadRequestedProfile
  }
}
