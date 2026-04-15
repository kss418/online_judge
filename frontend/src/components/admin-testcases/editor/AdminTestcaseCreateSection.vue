<template>
  <article class="admin-testcases-section admin-testcases-section--append">
    <div class="panel-header">
      <div>
        <p class="panel-kicker">append</p>
        <h3>테스트케이스 추가</h3>
      </div>
    </div>

    <div class="admin-testcases-grid">
      <AdminTextareaPanel
        :model-value="section.model.newTestcaseInput"
        label="입력"
        placeholder="빈 입력도 허용됩니다."
        :disabled="Boolean(section.model.busySection)"
        @update:model-value="section.actions.updateNewTestcaseInput"
      />

      <AdminTextareaPanel
        :model-value="section.model.newTestcaseOutput"
        label="출력"
        placeholder="빈 출력도 허용됩니다."
        :disabled="Boolean(section.model.busySection)"
        @update:model-value="section.actions.updateNewTestcaseOutput"
      />
    </div>

    <div class="admin-testcases-actions">
      <button
        type="button"
        class="primary-button"
        :disabled="!section.model.canCreateTestcase"
        @click="handleCreateTestcase"
      >
        {{ section.model.isCreatingTestcase ? '추가 중...' : '테스트케이스 추가' }}
      </button>
    </div>
  </article>
</template>

<script setup>
import { computed } from 'vue'

import AdminTextareaPanel from '@/components/adminShared/AdminTextareaPanel.vue'

const props = defineProps({
  section: {
    type: Object,
    required: true
  }
})

const section = computed(() => props.section)

function handleCreateTestcase(){
  section.value.actions.createTestcase()
}
</script>

<style scoped>
.admin-testcases-section {
  display: grid;
  gap: 1rem;
  padding: 1.2rem;
  border: 1px solid var(--admin-testcases-section-border);
  border-radius: 20px;
  background: var(--admin-testcases-section-surface);
  box-shadow: var(--admin-testcases-section-shadow);
}

.admin-testcases-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 1rem;
}

.admin-testcases-actions {
  display: flex;
  justify-content: flex-end;
  margin-top: 1rem;
}

@media (max-width: 720px) {
  .admin-testcases-grid {
    grid-template-columns: 1fr;
  }

  .admin-testcases-actions {
    justify-content: stretch;
  }
}
</style>
