<template>
  <div class="admin-testcases-toolbar">
    <div>
      <p class="panel-kicker">Testcase Admin</p>
      <h3>테스트케이스 관리</h3>
      <p class="admin-testcases-copy">
        왼쪽에서 문제를 고른 뒤 테스트케이스를 확인하고, 마지막 항목 추가와 삭제, 선택 항목 수정까지 한 화면에서 처리할 수 있습니다.
      </p>
    </div>

    <div class="admin-testcases-toolbar-actions">
      <StatusBadge
        :label="toolbar.model.statusLabel"
        :tone="toolbar.model.statusTone"
      />
      <button
        v-if="toolbar.model.canManageProblems"
        type="button"
        class="ghost-button"
        :disabled="Boolean(toolbar.model.busySection) || toolbar.model.isLoadingProblems || toolbar.model.isLoadingProblem || toolbar.model.isLoadingTestcases"
        @click="toolbar.actions.refresh()"
      >
        새로고침
      </button>
      <RouterLink
        v-if="toolbar.model.selectedProblemId > 0"
        class="ghost-button"
        :to="{ name: 'problem-detail', params: { problemId: toolbar.model.selectedProblemId } }"
      >
        문제 보기
      </RouterLink>
    </div>
  </div>
</template>

<script setup>
import StatusBadge from '@/components/StatusBadge.vue'

defineProps({
  toolbar: {
    type: Object,
    required: true
  }
})
</script>

<style scoped>
.admin-testcases-toolbar {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: start;
  flex-wrap: wrap;
}

.admin-testcases-copy {
  margin: 0.55rem 0 0;
  color: var(--ink-soft);
}

.admin-testcases-toolbar-actions {
  display: flex;
  flex-wrap: wrap;
  justify-content: flex-end;
  gap: 0.75rem;
  align-items: center;
}
</style>
