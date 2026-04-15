import { computed, ref } from 'vue'

import { useProtectedAdminPageAccess } from '@/composables/adminShared/useProtectedAdminPageAccess'
import { useAdminUserSubmissionBanActions } from '@/composables/adminUsers/useAdminUserSubmissionBanActions'
import { useUserListResource } from '@/composables/users/useUserListResource'
import { useUserSearchPagination } from '@/composables/users/useUserSearchPagination'
import { usePollingController } from '@/composables/usePollingController'
import { authStore } from '@/stores/auth/authStore'
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
  const userManagementListResource = useUserListResource({
    mode: 'management',
    authState,
    canLoad: canManageUsers
  })
  const searchPagination = useUserSearchPagination({
    users: userManagementListResource.users,
    searchMode: 'local'
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
  const emptyMessage = computed(() =>
    searchPagination.appliedQuery.value
      ? '검색 결과가 없습니다.'
      : '표시할 사용자가 아직 없습니다.'
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

  const pageAccess = useProtectedAdminPageAccess({
    authState,
    initializeAuth,
    isAuthenticated,
    hasAccess: canManageUsers,
    onDenied: userManagementListResource.resetUsers,
    onAllowed: userManagementListResource.loadUsers,
    loggedOutMessage: '유저 관리 페이지는 로그인한 관리자만 사용할 수 있습니다.',
    deniedMessage: '이 페이지는 관리자만 접근할 수 있습니다.'
  })
  const viewState = computed(() => {
    if (pageAccess.accessState.value === 'initializing' || pageAccess.accessState.value === 'logged-out') {
      return 'notice'
    }

    if (pageAccess.accessState.value === 'denied') {
      return 'denied'
    }

    if (isLoading.value) {
      return 'loading'
    }

    if (userManagementListResource.errorMessage.value) {
      return 'error'
    }

    if (!searchPagination.filteredUsers.value.length) {
      return 'empty'
    }

    return 'ready'
  })
  const viewMessage = computed(() => {
    if (!pageAccess.canAccessPage.value) {
      return pageAccess.accessMessage.value
    }

    if (isLoading.value) {
      return '유저 목록을 불러오는 중입니다.'
    }

    if (userManagementListResource.errorMessage.value) {
      return userManagementListResource.errorMessage.value
    }

    return ''
  })

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
    isBusyUser: submissionBanActions.isBusyUser,
    viewState,
    viewMessage,
    emptyMessage
  }
}
