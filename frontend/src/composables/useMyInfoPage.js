import { computed, onMounted, ref, watch } from 'vue'
import { useRoute } from 'vue-router'

import { authStore } from '@/stores/auth/authStore'
import { usePollingController } from '@/composables/usePollingController'
import { useMyProfileResource } from '@/composables/myInfo/useMyProfileResource'
import { useMyRecentSubmissionsResource } from '@/composables/myInfo/useMyRecentSubmissionsResource'
import { useMySolvedProblemsResource } from '@/composables/myInfo/useMySolvedProblemsResource'
import { useMyStatisticsResource } from '@/composables/myInfo/useMyStatisticsResource'
import { useMySubmissionBanResource } from '@/composables/myInfo/useMySubmissionBanResource'
import { useMyWrongProblemsResource } from '@/composables/myInfo/useMyWrongProblemsResource'
import { formatRelativeTimestamp, formatTimestamp } from '@/utils/dateTime'
import { getProblemStateTextClass } from '@/utils/problemState'
import {
  getSubmissionStatisticsFieldName,
  submissionStatisticsVisibleCatalog
} from '@/generated/submissionStatusCatalog'

export function useMyInfoPage(){
  const route = useRoute()
  const {
    state: authState,
    isAuthenticated,
    initializeAuth
  } = authStore
  const nowTimestamp = ref(Date.now())
  const profile = useMyProfileResource({
    route,
    authState,
    isAuthenticated
  })
  const submissionBan = useMySubmissionBanResource({
    authState,
    isOwnProfile: profile.isOwnProfile,
    nowTimestamp
  })
  const statistics = useMyStatisticsResource()
  const recentSubmissions = useMyRecentSubmissionsResource({
    authState
  })
  const solvedProblems = useMySolvedProblemsResource({
    authState
  })
  const wrongProblems = useMyWrongProblemsResource({
    authState
  })

  usePollingController({
    task(){
      nowTimestamp.value = Date.now()
    },
    enabled: true,
    intervalMs: 30_000,
    pauseWhenHidden: false,
    runImmediately: true
  })

  const statisticsStatusLabel = computed(() => {
    if (profile.shouldShowLoadingState.value || statistics.isStatisticsLoading.value) {
      return 'Loading'
    }

    if (!profile.showExtendedProfilePanels.value) {
      return 'Guest'
    }

    return statistics.statisticsErrorMessage.value ? 'Error' : 'Loaded'
  })
  const statisticsStatusTone = computed(() => {
    if (profile.shouldShowLoadingState.value || statistics.isStatisticsLoading.value) {
      return 'neutral'
    }

    if (!profile.showExtendedProfilePanels.value) {
      return 'neutral'
    }

    if (statistics.statisticsErrorMessage.value) {
      return 'danger'
    }

    return 'success'
  })
  const solvedProblemsStatusLabel = computed(() => {
    if (profile.shouldShowLoadingState.value || solvedProblems.isSolvedProblemsLoading.value) {
      return 'Loading'
    }

    if (!profile.showExtendedProfilePanels.value) {
      return 'Guest'
    }

    if (solvedProblems.solvedProblemsErrorMessage.value) {
      return 'Error'
    }

    return `${solvedProblems.solvedProblems.value.length} Solved`
  })
  const solvedProblemsStatusTone = computed(() => {
    if (profile.shouldShowLoadingState.value || solvedProblems.isSolvedProblemsLoading.value) {
      return 'neutral'
    }

    if (!profile.showExtendedProfilePanels.value) {
      return 'neutral'
    }

    if (solvedProblems.solvedProblemsErrorMessage.value) {
      return 'danger'
    }

    return 'success'
  })
  const wrongProblemsStatusLabel = computed(() => {
    if (profile.shouldShowLoadingState.value || wrongProblems.isWrongProblemsLoading.value) {
      return 'Loading'
    }

    if (!profile.showExtendedProfilePanels.value) {
      return 'Guest'
    }

    if (wrongProblems.wrongProblemsErrorMessage.value) {
      return 'Error'
    }

    return `${wrongProblems.wrongProblems.value.length} Wrong`
  })
  const wrongProblemsStatusTone = computed(() => {
    if (profile.shouldShowLoadingState.value || wrongProblems.isWrongProblemsLoading.value) {
      return 'neutral'
    }

    if (!profile.showExtendedProfilePanels.value) {
      return 'neutral'
    }

    if (wrongProblems.wrongProblemsErrorMessage.value) {
      return 'danger'
    }

    return 'danger'
  })
  const statisticsItems = computed(() => {
    const defaultStatusCounts = Object.fromEntries(
      submissionStatisticsVisibleCatalog.map((statusMeta) => [
        getSubmissionStatisticsFieldName(statusMeta.code),
        0
      ])
    )
    const currentStatistics = statistics.submissionStatistics.value ?? {
      submission_count: 0,
      ...defaultStatusCounts,
      last_submission_at: null,
      last_accepted_at: null
    }
    const statusItems = submissionStatisticsVisibleCatalog.map((statusMeta) => ({
      label: statusMeta.public_label_ko,
      value: currentStatistics[getSubmissionStatisticsFieldName(statusMeta.code)] ?? 0,
      status: statusMeta.code
    }))

    return [
      {
        label: '전체 제출',
        value: currentStatistics.submission_count
      },
      ...statusItems,
      {
        label: '최근 제출',
        value: formatTimestamp(currentStatistics.last_submission_at)
      },
      {
        label: '최근 정답',
        value: formatTimestamp(currentStatistics.last_accepted_at)
      }
    ]
  })

  function formatRelativeSubmittedAt(timestamp, fallbackLabel = '-'){
    return formatRelativeTimestamp(nowTimestamp.value, timestamp, fallbackLabel || '-')
  }

  watch(
    [
      profile.requestedProfileUserLoginId,
      profile.isUserProfileRoute,
      () => authState.initialized
    ],
    ([profileUserLoginId]) => {
      if (!profileUserLoginId) {
        profile.resetProfile()

        if (profile.isUserProfileRoute.value) {
          profile.publicUserSummaryErrorMessage.value = '유효하지 않은 사용자입니다.'
        }

        return
      }

      void profile.loadRequestedProfile(profileUserLoginId)
    },
    {
      immediate: true
    }
  )

  watch(
    [
      profile.activeProfileUserId,
      () => authState.initialized,
      () => authState.token,
      () => profile.currentUser.value.id
    ],
    ([profileUserId]) => {
      if (profileUserId <= 0) {
        statistics.resetSubmissionStatistics()
        recentSubmissions.resetRecentSubmissions()
        solvedProblems.resetSolvedProblems()
        wrongProblems.resetWrongProblems()
        return
      }

      void statistics.loadSubmissionStatistics(profileUserId)
      void recentSubmissions.loadRecentSubmissions(profileUserId)
      void solvedProblems.loadSolvedProblems(profileUserId)
      void wrongProblems.loadWrongProblems(profileUserId)
    },
    {
      immediate: true
    }
  )

  watch(
    [
      profile.isOwnProfile,
      () => authState.initialized,
      () => authState.token,
      () => profile.currentUser.value.id
    ],
    ([ownProfile, initialized]) => {
      if (!initialized || !ownProfile || !authState.token) {
        submissionBan.resetMySubmissionBan()
        return
      }

      void submissionBan.loadMySubmissionBan()
    },
    {
      immediate: true
    }
  )

  onMounted(() => {
    void initializeAuth()
  })

  return {
    authState,
    isAuthenticated,
    showExtendedProfilePanels: profile.showExtendedProfilePanels,
    shouldShowLoadingState: profile.shouldShowLoadingState,
    isStatisticsLoading: statistics.isStatisticsLoading,
    statisticsErrorMessage: statistics.statisticsErrorMessage,
    statisticsItems,
    statisticsStatusLabel,
    statisticsStatusTone,
    recentSubmissions: recentSubmissions.recentSubmissions,
    isRecentSubmissionsLoading: recentSubmissions.isRecentSubmissionsLoading,
    recentSubmissionsErrorMessage: recentSubmissions.recentSubmissionsErrorMessage,
    isProfileLoading: profile.isProfileLoading,
    profileErrorMessage: profile.profileErrorMessage,
    displayedUser: profile.displayedUser,
    isOwnProfile: profile.isOwnProfile,
    mySubmissionBan: submissionBan.mySubmissionBan,
    mySubmissionBanStatusLabel: submissionBan.mySubmissionBanStatusLabel,
    mySubmissionBanStatusTone: submissionBan.mySubmissionBanStatusTone,
    mySubmissionBanWindowText: submissionBan.mySubmissionBanWindowText,
    mySubmissionBanUntilText: submissionBan.mySubmissionBanUntilText,
    solvedProblems: solvedProblems.solvedProblems,
    isSolvedProblemsLoading: solvedProblems.isSolvedProblemsLoading,
    solvedProblemsErrorMessage: solvedProblems.solvedProblemsErrorMessage,
    solvedProblemsStatusLabel,
    solvedProblemsStatusTone,
    wrongProblems: wrongProblems.wrongProblems,
    isWrongProblemsLoading: wrongProblems.isWrongProblemsLoading,
    wrongProblemsErrorMessage: wrongProblems.wrongProblemsErrorMessage,
    wrongProblemsStatusLabel,
    wrongProblemsStatusTone,
    profileStatusLabel: profile.profileStatusLabel,
    profileStatusTone: profile.profileStatusTone,
    formatRelativeSubmittedAt,
    formatTimestamp,
    getProblemStateTextClass
  }
}
