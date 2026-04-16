<template>
  <section class="admin-problem-editor-panel">
    <div
      v-if="editor.shell.mode !== 'ready'"
      class="empty-state"
      :class="{ 'error-state': editor.shell.mode === 'error' }"
    >
      <p>{{ editor.shell.message }}</p>
    </div>

    <template v-else-if="editor.shell.mode === 'ready'">
      <AdminProblemEditorHeader :section="editor.sections.header" />

      <AdminProblemBasicsSection :section="editor.sections.basics" />

      <AdminProblemStatementSection :section="editor.sections.statement" />

      <AdminProblemSamplesSection :section="editor.sections.samples" />

      <AdminProblemTestcaseUploadSection :section="editor.sections.testcaseUpload" />

      <AdminProblemDangerSection :section="editor.sections.danger" />
    </template>
  </section>
</template>

<script setup>
import { computed } from 'vue'

import AdminProblemBasicsSection from '@/components/admin-problems/editor/AdminProblemBasicsSection.vue'
import AdminProblemDangerSection from '@/components/admin-problems/editor/AdminProblemDangerSection.vue'
import AdminProblemEditorHeader from '@/components/admin-problems/editor/AdminProblemEditorHeader.vue'
import AdminProblemSamplesSection from '@/components/admin-problems/editor/AdminProblemSamplesSection.vue'
import AdminProblemStatementSection from '@/components/admin-problems/editor/AdminProblemStatementSection.vue'
import AdminProblemTestcaseUploadSection from '@/components/admin-problems/editor/AdminProblemTestcaseUploadSection.vue'

const props = defineProps({
  editor: {
    type: Object,
    required: true
  }
})

const editor = computed(() => props.editor)
</script>

<style scoped>
.admin-problem-editor-panel {
  display: grid;
  gap: 1rem;
  min-height: 40rem;
  padding: 1rem;
  border: 1px solid var(--admin-problems-shell-border);
  border-radius: 24px;
  background: var(--admin-problems-shell-surface);
  box-shadow: var(--admin-problems-shell-shadow);
}
</style>
