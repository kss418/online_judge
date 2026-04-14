import { computed, onMounted, ref, watch } from 'vue'

import { authStore } from '@/stores/auth/authStore'
import { useAdminUserManagementListResource } from '@/composables/adminUsers/useAdminUserManagementListResource'
import { useAdminUserSearchPagination } from '@/composables/adminUsers/useAdminUserSearchPagination'
import { useAdminUserSubmissionBanActions } from '@/composables/adminUsers/useAdminUserSubmissionBanActions'
import { usePollingController } from '@/composables/usePollingController'
import { formatCount as formatNumberCount } from '@/utils/numberFormat'

const koreanNumberFormatOptions = {
  locale: 'ko-KR'
}

function formatTimeDistance(distanceMs){
  const totalSeconds = Math.max(1, Math.floor(distanceMs / 1000))

  if (totalSeconds < 60) {
    return `${totalSeconds}초`
  }

  const totalMinutes = Math.floor(totalSeconds / 60)
  if (totalMinutes < 60) {
    return `${totalMinutes}분`
  }

  const totalHours = Math.floor(totalMinutes / 60)
  if (totalHours < 24) {
    return `${totalHours}시간`
  }

  const totalDays = Math.floor(totalHours / 24)
  if (totalDays < 30) {
    return `${totalDays}일`
  }

  const totalMonths = Math.floor(totalDays / 30)
  if (totalMonths < 12) {
    return `${totalMonths}달`
  }

  return `${Math.floor(totalDays / 365)}년`
}

export function useAdminUserManagementPage(){
  const {
    state: authState,
    isAuthenticated,
    initializeAuth
  } = authStore
  const canManageUsers = computed(() => Number(authState.currentUser?.permission_level ?? 0) >= 1)
  const nowTimestamp = ref(Date.now())
  const userManagementListResource = useAdminUserManagementListResource({
    authState,
    canManageUsers
  })
  const searchPagination = useAdminUserSearchPagination({
    users: userManagementListResource.users
  })
  const submissionBanActions = useAdminUserSubmissionBanActions({
    authState,
    canManageUsers,
    patchUser: userManagementListResource.patchUser
  })
  const isLoading = computed(() =>
    !authState.initialized || userManagementListResource.isLoading.value
  )
  const activeBanCount = computed(() =>
    userManagementListResource.users.value.filter((user) => getSubmissionBanState(user) === 'active').length
  )
  const normalUserCount = computed(() =>
    userManagementListResource.users.value.filter((user) => getSubmissionBanState(user) === 'none').length
  )
  const formatCount = (value) => formatNumberCount(value, koreanNumberFormatOptions)

  usePollingController({
    task(){
      nowTimestamp.value = Date.now()
    },
    enabled: true,
    intervalMs: 1000,
    pauseWhenHidden: false,
    runImmediately: true
  })

  watch(
    [
      () => authState.initialized,
      () => authState.token,
      isAuthenticated,
      canManageUsers
    ],
    ([initialized, token, authenticated, canManage]) => {
      if (!initialized) {
        return
      }

      if (!authenticated || !canManage || !token) {
        userManagementListResource.resetUsers()
        return
      }

      void userManagementListResource.loadUsers()
    },
    {
      immediate: true
    }
  )

  function loadUsers(){
    return userManagementListResource.loadUsers()
  }

  function getSubmissionBanState(user){
    if (user.submission_ban_status_loading) {
      return 'loading'
    }

    if (user.submission_ban_status_error) {
      return 'error'
    }

    if (
      typeof user.submission_banned_until_timestamp !== 'number' ||
      Number.isNaN(user.submission_banned_until_timestamp)
    ) {
      return 'none'
    }

    if (user.submission_banned_until_timestamp > nowTimestamp.value) {
      return 'active'
    }

    return 'none'
  }

  function getSubmissionBanLabel(user){
    const state = getSubmissionBanState(user)

    if (state === 'loading') {
      return '확인 중'
    }

    if (state === 'error') {
      return '조회 실패'
    }

    if (state === 'active') {
      return '제출 금지'
    }

    return '정상'
  }

  function getSubmissionBanTone(user){
    const state = getSubmissionBanState(user)

    if (state === 'loading') {
      return 'neutral'
    }

    if (state === 'error') {
      return 'warning'
    }

    if (state === 'active') {
      return 'danger'
    }

    return 'success'
  }

  function formatSubmissionBanWindow(user){
    const state = getSubmissionBanState(user)

    if (state === 'loading') {
      return '상태 확인 중'
    }

    if (state === 'error') {
      return '다시 새로고침해 주세요'
    }

    if (state === 'none') {
      return ''
    }

    const timestamp = user.submission_banned_until_timestamp

    if (typeof timestamp !== 'number' || Number.isNaN(timestamp)) {
      return '시각 확인 불가'
    }

    if (state === 'active') {
      return `${formatTimeDistance(Math.abs(timestamp - nowTimestamp.value))} 남음`
    }

    return ''
  }

  function shouldShowSubmissionBanUntil(user){
    return Boolean(
      getSubmissionBanState(user) === 'active' &&
      user.submission_banned_until_label
    )
  }

  function getCreateBanButtonLabel(user){
    if (submissionBanActions.isBusyUser(user.user_id) && submissionBanActions.actionType.value === 'create') {
      return '적용 중...'
    }

    return getSubmissionBanState(user) === 'active' ? '기간 갱신' : '밴 설정'
  }

  function getClearBanButtonLabel(user){
    if (submissionBanActions.isBusyUser(user.user_id) && submissionBanActions.actionType.value === 'clear') {
      return '해제 중...'
    }

    return '밴 해제'
  }

  onMounted(() => {
    void initializeAuth()
  })

  return {
    authState,
    isAuthenticated,
    canManageUsers,
    formatCount,
    pageSize: searchPagination.pageSize,
    durationPresets: submissionBanActions.durationPresets,
    isLoading,
    errorMessage: userManagementListResource.errorMessage,
    users: userManagementListResource.users,
    filteredUsers: searchPagination.filteredUsers,
    pagedUsers: searchPagination.pagedUsers,
    searchInput: searchPagination.searchInput,
    appliedQuery: searchPagination.appliedQuery,
    currentPage: searchPagination.currentPage,
    totalPages: searchPagination.totalPages,
    pageJumpInput: searchPagination.pageJumpInput,
    paginationItems: searchPagination.paginationItems,
    activeBanCount,
    normalUserCount,
    loadUsers,
    submitSearch: searchPagination.submitSearch,
    resetSearch: searchPagination.resetSearch,
    goToPage: searchPagination.goToPage,
    submitPageJump: searchPagination.submitPageJump,
    getSubmissionBanLabel,
    getSubmissionBanTone,
    formatSubmissionBanWindow,
    shouldShowSubmissionBanUntil,
    getDurationDraft: submissionBanActions.getDurationDraft,
    setDurationDraft: submissionBanActions.setDurationDraft,
    updateDurationDraft: submissionBanActions.updateDurationDraft,
    handleCreateSubmissionBan: submissionBanActions.handleCreateSubmissionBan,
    handleClearSubmissionBan: submissionBanActions.handleClearSubmissionBan,
    getCreateBanButtonLabel,
    getClearBanButtonLabel,
    isBusyUser: submissionBanActions.isBusyUser
  }
}
