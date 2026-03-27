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
        :label="toolbarStatusLabel"
        :tone="toolbarStatusTone"
      />
      <button
        v-if="canManageProblems"
        type="button"
        class="ghost-button"
        :disabled="Boolean(busySection) || isLoadingProblems || isLoadingProblem || isLoadingTestcases"
        @click="$emit('refresh')"
      >
        새로고침
      </button>
      <RouterLink
        v-if="selectedProblemId > 0"
        class="ghost-button"
        :to="{ name: 'problem-detail', params: { problemId: selectedProblemId } }"
      >
        문제 보기
      </RouterLink>
    </div>
  </div>
</template>

<script setup>
import StatusBadge from '@/components/StatusBadge.vue'

defineProps({
  toolbarStatusLabel: {
    type: String,
    required: true
  },
  toolbarStatusTone: {
    type: String,
    required: true
  },
  canManageProblems: {
    type: Boolean,
    required: true
  },
  busySection: {
    type: String,
    default: ''
  },
  isLoadingProblems: {
    type: Boolean,
    required: true
  },
  isLoadingProblem: {
    type: Boolean,
    required: true
  },
  isLoadingTestcases: {
    type: Boolean,
    required: true
  },
  selectedProblemId: {
    type: Number,
    required: true
  }
})

defineEmits(['refresh'])
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
