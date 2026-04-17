import { computed } from 'vue'

import { normalizeAdminProblemId } from '@/composables/adminShared/adminProblemSelectionHelpers'
import { useAdminProblemSidebarModel } from '@/composables/adminShared/useAdminProblemSidebarModel'
import { useProtectedAdminPageAccess } from '@/composables/adminShared/useProtectedAdminPageAccess'
import { useAdminProblemToolbarState } from '@/composables/adminShared/useAdminProblemToolbarState'

function normalizeCallbackList(callbacks){
  if (!Array.isArray(callbacks)) {
    return []
  }

  return callbacks.filter((callback) => typeof callback === 'function')
}

async function runCallbacksSequentially(callbacks, ...args){
  for (const callback of callbacks) {
    await callback(...args)
  }
}

function findFirstNonSuccessResult(results){
  return results.find((result) =>
    result &&
    typeof result === 'object' &&
    'status' in result &&
    result.status !== 'success'
  ) || null
}

export function useAdminProblemSelectionPageState({
  core,
  authState,
  initializeAuth,
  isAuthenticated,
  canManageProblems,
  accessMessages,
  busySection,
  canRefresh = () => true,
  selectionResetters = [],
  beforeSelectedProblemLoad,
  afterSelectedProblemDetailLoad,
  additionalSelectedProblemLoaders = [],
  pageResetters = []
}){
  const normalizedSelectionResetters = normalizeCallbackList(selectionResetters)
  const normalizedAdditionalLoaders = normalizeCallbackList(additionalSelectedProblemLoaders)
  const normalizedPageResetters = normalizeCallbackList(pageResetters)
  const canRefreshWorkspace = typeof canRefresh === 'function'
    ? canRefresh
    : () => true

  async function resetSelectedProblemState(context = {}){
    core.problemDetailResource.resetSelectedProblemDetail()
    await runCallbacksSequentially(normalizedSelectionResetters, context)
  }

  async function loadSelectedProblemData(problemId = core.selectedProblemId.value, context = {}){
    const normalizedProblemId = normalizeAdminProblemId(problemId)

    if (!normalizedProblemId) {
      await resetSelectedProblemState(context)
      return {
        status: 'reset'
      }
    }

    if (typeof beforeSelectedProblemLoad === 'function') {
      await beforeSelectedProblemLoad(normalizedProblemId, context)
    }

    await resetSelectedProblemState(context)

    const [detailResult, ...additionalResults] = await Promise.all([
      core.problemDetailResource.loadProblemDetail(normalizedProblemId),
      ...normalizedAdditionalLoaders.map((loader) => loader(normalizedProblemId, context))
    ])

    if (detailResult.status !== 'success') {
      return detailResult
    }

    if (typeof afterSelectedProblemDetailLoad === 'function') {
      await afterSelectedProblemDetailLoad(detailResult.data, normalizedProblemId, context)
    }

    const firstNonSuccessResult = findFirstNonSuccessResult(additionalResults)
    if (firstNonSuccessResult) {
      return firstNonSuccessResult
    }

    return detailResult
  }

  async function resetPageState(){
    core.query.resetSearchControls()
    busySection.value = ''
    core.problemCatalogResource.resetProblems()
    await resetSelectedProblemState()
    await runCallbacksSequentially(normalizedPageResetters)
  }

  async function handleAllowedAccess(){
    const previousSelectedProblemId = core.selectedProblemId.value

    await core.query.syncFromRouteAndReload()

    if (core.selectedProblemId.value !== previousSelectedProblemId) {
      return
    }

    return loadSelectedProblemData()
  }

  const pageAccess = useProtectedAdminPageAccess({
    authState,
    initializeAuth,
    isAuthenticated,
    hasAccess: canManageProblems,
    onDenied: resetPageState,
    onAllowed: handleAllowedAccess,
    loggedOutMessage: accessMessages.loggedOutMessage,
    deniedMessage: accessMessages.deniedMessage
  })

  pageAccess.watchWhenAllowed(
    () => [
      core.query.routeSearchMode.value,
      core.query.routeTitleSearch.value,
      core.query.routeProblemIdSearch.value
    ],
    () => {
      void core.query.syncFromRouteAndReload()
    }
  )

  pageAccess.watchWhenAllowed(core.selectedProblemId, (problemId) => {
    void loadSelectedProblemData(problemId)
  })

  async function refreshWorkspace(){
    if (!canRefreshWorkspace()) {
      return
    }

    const previousSelectedProblemId = core.selectedProblemId.value

    await core.loadProblems({
      preferredProblemId: core.query.preferredProblemIdForReload.value
    })

    if (core.selectedProblemId.value !== previousSelectedProblemId) {
      return
    }

    return loadSelectedProblemData()
  }

  const isLoadingProblems = computed(() =>
    pageAccess.accessState.value === 'initializing' || core.problemCatalogResource.isLoadingProblems.value
  )
  const shell = computed(() => ({
    state: pageAccess.accessState.value,
    message: pageAccess.accessMessage.value
  }))

  return {
    pageAccess,
    shell,
    isLoadingProblems,
    refreshWorkspace,
    resetSelectedProblemState,
    loadSelectedProblemData,
    resetPageState
  }
}

export function useAdminProblemSelectionPageShell({
  workspace,
  canManageProblems,
  busySection,
  statusLabel,
  statusTone,
  toolbarExtraModel,
  formatCount,
  titleSearchInputId,
  problemIdSearchInputId,
  sidebarCreate
}){
  const toolbar = useAdminProblemToolbarState({
    workspace,
    canManageProblems,
    busySection,
    statusLabel,
    statusTone,
    extraModel: toolbarExtraModel
  })
  const sidebar = useAdminProblemSidebarModel({
    workspace,
    busySection,
    formatCount,
    titleSearchInputId,
    problemIdSearchInputId,
    create: sidebarCreate
  })

  return {
    shell: workspace.shell,
    toolbar,
    sidebar
  }
}
