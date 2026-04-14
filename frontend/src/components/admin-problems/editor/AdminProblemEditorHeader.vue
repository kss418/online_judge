<template>
  <div class="admin-problem-editor-header">
    <div>
      <p class="panel-kicker">editor</p>
      <h3 class="admin-problem-heading">
        <span class="admin-problem-heading-id-group">
          <span class="admin-problem-heading-id">
            #{{ section.model.formatCount(section.model.selectedProblemDetail.problem_id) }}
          </span>
          <span class="admin-problem-heading-divider" aria-hidden="true">·</span>
        </span>
        <span class="admin-problem-heading-text">{{ section.model.selectedProblemDetail.title }}</span>
      </h3>
      <p class="admin-problem-editor-copy">
        선택한 문제의 제목, 제한, 설명, 공개 예제를 바로 저장할 수 있습니다.
      </p>
    </div>

    <div class="admin-problem-editor-header-actions">
      <button
        type="button"
        class="ghost-button admin-problem-rejudge-button"
        :disabled="Boolean(section.model.busySection)"
        @click="handleOpenRejudgeDialog"
      >
        재채점
      </button>
      <RouterLink
        class="ghost-button"
        :to="{ name: 'admin-problem-testcases', params: { problemId: section.model.selectedProblemDetail.problem_id } }"
      >
        테스트케이스 관리
      </RouterLink>
      <RouterLink
        class="ghost-button"
        :to="{ name: 'problem-detail', params: { problemId: section.model.selectedProblemDetail.problem_id } }"
      >
        문제 보기
      </RouterLink>
    </div>
  </div>
</template>

<script setup>
import { computed } from 'vue'

const props = defineProps({
  section: {
    type: Object,
    required: true
  }
})

const section = computed(() => props.section)

function handleOpenRejudgeDialog(){
  section.value.actions.openRejudgeDialog()
}
</script>

<style scoped>
.admin-problem-editor-header {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: start;
}

.admin-problem-heading {
  display: flex;
  align-items: baseline;
  gap: 0.75rem;
  flex-wrap: wrap;
  min-width: 0;
  margin: 0;
}

.admin-problem-heading-id-group {
  display: inline-flex;
  align-items: baseline;
  gap: 0.5rem;
  white-space: nowrap;
  color: var(--ink-soft);
  flex-shrink: 0;
}

.admin-problem-heading-id {
  font-weight: 800;
  letter-spacing: -0.03em;
}

.admin-problem-heading-divider {
  color: rgba(20, 33, 61, 0.34);
  font-weight: 700;
}

.admin-problem-heading-text {
  min-width: 0;
  color: var(--ink-strong);
}

.admin-problem-editor-copy {
  margin: 0;
  color: var(--ink-soft);
}

.admin-problem-editor-header-actions {
  display: flex;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
}

.admin-problem-rejudge-button {
  min-width: 6.25rem;
  padding-inline: 0.95rem;
  color: var(--warning);
  background: rgba(255, 247, 237, 0.96);
  border-color: rgba(180, 83, 9, 0.18);
}

@media (max-width: 1100px) {
  .admin-problem-editor-header {
    flex-direction: column;
  }
}
</style>
