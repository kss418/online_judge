import { useAdminProblemSidebarModel } from '@/composables/adminShared/useAdminProblemSidebarModel'
import { useAdminProblemSelectionWorkspaceEffects } from '@/composables/adminShared/useAdminProblemSelectionWorkspace'
import { useAdminProblemToolbarState } from '@/composables/adminShared/useAdminProblemToolbarState'

export function useAdminProblemSelectionPageWorkspace({
  core,
  authState,
  initializeAuth,
  isAuthenticated,
  canManageProblems,
  accessMessages,
  canRefresh,
  resetSelectedProblemState,
  loadSelectedProblemData,
  resetPageState
}){
  const effects = useAdminProblemSelectionWorkspaceEffects({
    core,
    authState,
    initializeAuth,
    isAuthenticated,
    canManageProblems,
    accessMessages,
    canRefresh,
    resetSelectedProblemState,
    loadSelectedProblemData,
    resetPageState
  })

  return {
    ...core,
    ...effects
  }
}

export async function resetAdminProblemSelectionPageState({
  workspaceCore,
  busySection,
  resetSelectedProblemState
}){
  workspaceCore.query.resetSearchControls()
  busySection.value = ''
  workspaceCore.problemCatalogResource.resetProblems()
  await resetSelectedProblemState()
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
