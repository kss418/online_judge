<template>
  <AdminSplitWorkspaceShell
    :access-state="shell.state"
    :access-message="shell.message"
  >
    <template #toolbar>
      <AdminTestcasesToolbar :toolbar="toolbar" />
    </template>

    <template #sidebar>
      <AdminProblemSelectionSidebar :sidebar="sidebar" />
    </template>

    <AdminTestcasesEditor :editor="editor" />
  </AdminSplitWorkspaceShell>
</template>

<script setup>
import AdminProblemSelectionSidebar from '@/components/adminShared/AdminProblemSelectionSidebar.vue'
import AdminSplitWorkspaceShell from '@/components/adminShared/AdminSplitWorkspaceShell.vue'
import AdminTestcasesEditor from '@/components/admin-testcases/AdminTestcasesEditor.vue'
import AdminTestcasesToolbar from '@/components/admin-testcases/AdminTestcasesToolbar.vue'
import { useAdminProblemTestcasesPage } from '@/composables/useAdminProblemTestcasesPage'

const {
  shell,
  toolbar,
  sidebar,
  editor
} = useAdminProblemTestcasesPage()
</script>
