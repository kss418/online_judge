<template>
  <div class="admin-testcases-editor-header">
    <div>
      <p class="panel-kicker">editor</p>
      <h3 class="admin-testcases-problem-heading">
        <span class="admin-testcases-problem-heading-id-group">
          <span class="admin-testcases-problem-heading-id">
            #{{ section.model.formatCount(section.model.problemDetail.problem_id) }}
          </span>
          <span class="admin-testcases-problem-heading-divider" aria-hidden="true">·</span>
        </span>
        <span class="admin-testcases-problem-heading-text">{{ section.model.problemDetail.title }}</span>
      </h3>
      <p class="admin-testcases-editor-copy">
        요약 리스트에서 특정 테스트케이스를 고르고, 선택한 항목만 편집해서 저장할 수 있습니다.
      </p>
      <p
        v-if="section.model.testcaseItems.length > 1"
        class="admin-testcases-editor-copy"
      >
        요약 카드들을 드래그해서 순서를 바로 바꿀 수 있습니다.
      </p>
    </div>

    <div class="admin-testcases-editor-side">
      <div class="admin-testcases-editor-actions">
        <RouterLink
          class="ghost-button"
          :to="{
            name: 'admin-problems',
            query: { problemId: String(section.model.problemDetail.problem_id) }
          }"
        >
          문제 관리
        </RouterLink>
        <RouterLink
          class="ghost-button"
          :to="{ name: 'problem-detail', params: { problemId: section.model.problemDetail.problem_id } }"
        >
          문제 보기
        </RouterLink>
      </div>
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
</script>

<style scoped>
.admin-testcases-editor-header {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: start;
}

.admin-testcases-editor-copy {
  margin: 0.55rem 0 0;
  color: var(--ink-soft);
}

.admin-testcases-editor-copy + .admin-testcases-editor-copy {
  margin-top: 0.55rem;
}

.admin-testcases-problem-heading {
  display: flex;
  align-items: baseline;
  gap: 0.75rem;
  flex-wrap: wrap;
  min-width: 0;
}

.admin-testcases-problem-heading-id-group {
  display: inline-flex;
  align-items: baseline;
  gap: 0.5rem;
  white-space: nowrap;
  color: var(--ink-soft);
  flex-shrink: 0;
}

.admin-testcases-problem-heading-id {
  font-weight: 800;
  letter-spacing: -0.03em;
}

.admin-testcases-problem-heading-divider {
  color: rgba(20, 33, 61, 0.34);
  font-weight: 700;
}

.admin-testcases-problem-heading-text {
  min-width: 0;
  color: var(--ink-strong);
}

.admin-testcases-editor-side {
  display: grid;
  gap: 0.7rem;
  justify-items: end;
  flex: 0 0 auto;
}

.admin-testcases-editor-actions {
  display: flex;
  gap: 0.75rem;
  flex-wrap: wrap;
  justify-content: flex-end;
}

@media (max-width: 720px) {
  .admin-testcases-editor-header {
    align-items: start;
    flex-direction: column;
  }

  .admin-testcases-editor-side {
    width: 100%;
    justify-items: start;
  }

  .admin-testcases-editor-actions {
    justify-content: flex-start;
  }
}
</style>
