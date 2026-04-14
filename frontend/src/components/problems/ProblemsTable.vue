<template>
  <div class="problem-table">
    <div class="problem-table-head">
      <button
        type="button"
        class="problem-table-head-button problem-table-head-button--number"
        :class="{ 'is-active': appliedSortKey === 'problem_id' }"
        :title="getSortButtonTitle(problemSortOptionMap.problem_id)"
        @click="$emit('cycleSort', 'problem_id')"
      >
        <span class="problem-table-head-button-text">
          {{ problemSortOptionMap.problem_id.label }}
        </span>
        <span class="problem-table-head-button-indicator">
          {{ getSortButtonIndicator(problemSortOptionMap.problem_id.key) }}
        </span>
      </button>
      <span class="problem-table-head-label">제목</span>
      <div class="problem-table-stats-head" aria-label="문제 통계 정렬">
        <button
          type="button"
          class="problem-table-head-button"
          :class="{ 'is-active': appliedSortKey === 'accepted_count' }"
          :title="getSortButtonTitle(problemSortOptionMap.accepted_count)"
          @click="$emit('cycleSort', 'accepted_count')"
        >
          <span class="problem-table-head-button-text">
            {{ problemSortOptionMap.accepted_count.label }}
          </span>
          <span class="problem-table-head-button-indicator">
            {{ getSortButtonIndicator(problemSortOptionMap.accepted_count.key) }}
          </span>
        </button>
        <button
          type="button"
          class="problem-table-head-button"
          :class="{ 'is-active': appliedSortKey === 'submission_count' }"
          :title="getSortButtonTitle(problemSortOptionMap.submission_count)"
          @click="$emit('cycleSort', 'submission_count')"
        >
          <span class="problem-table-head-button-text">
            {{ problemSortOptionMap.submission_count.label }}
          </span>
          <span class="problem-table-head-button-indicator">
            {{ getSortButtonIndicator(problemSortOptionMap.submission_count.key) }}
          </span>
        </button>
        <button
          type="button"
          class="problem-table-head-button"
          :class="{ 'is-active': appliedSortKey === 'acceptance_rate' }"
          :title="getSortButtonTitle(problemSortOptionMap.acceptance_rate)"
          @click="$emit('cycleSort', 'acceptance_rate')"
        >
          <span class="problem-table-head-button-text">
            {{ problemSortOptionMap.acceptance_rate.label }}
          </span>
          <span class="problem-table-head-button-indicator">
            {{ getSortButtonIndicator(problemSortOptionMap.acceptance_rate.key) }}
          </span>
        </button>
      </div>
    </div>

    <RouterLink
      v-for="problem in problems"
      :key="problem.problem_id"
      class="problem-row problem-row-link"
      :to="{ name: 'problem-detail', params: { problemId: problem.problem_id } }"
    >
      <strong class="problem-id">#{{ problem.problem_id }}</strong>
      <div class="problem-main">
        <div class="problem-title-line">
          <strong>{{ problem.title }}</strong>
          <StatusBadge
            v-if="problem.user_problem_state"
            :label="getProblemStateLabel(problem.user_problem_state)"
            :tone="getProblemStateTone(problem.user_problem_state)"
          />
        </div>
      </div>
      <div class="problem-stats" aria-label="problem statistics">
        <strong class="problem-stat-value">
          {{ formatCount(problem.accepted_count) }}
        </strong>
        <strong class="problem-stat-value">
          {{ formatCount(problem.submission_count) }}
        </strong>
        <strong class="problem-rate-value">
          {{ formatAcceptanceRate(problem.accepted_count, problem.submission_count) }}
        </strong>
      </div>
    </RouterLink>
  </div>
</template>

<script setup>
import { computed } from 'vue'

import StatusBadge from '@/components/StatusBadge.vue'
import {
  formatAcceptanceRate,
  formatCount
} from '@/utils/numberFormat'
import {
  getProblemStateLabel,
  getProblemStateTone
} from '@/utils/problemState'

const props = defineProps({
  problems: {
    type: Array,
    required: true
  },
  problemSortOptions: {
    type: Array,
    required: true
  },
  appliedSortKey: {
    type: String,
    required: true
  },
  appliedSortDirection: {
    type: String,
    required: true
  }
})

defineEmits(['cycleSort'])

const problemSortOptionMap = computed(() =>
  Object.fromEntries(props.problemSortOptions.map((option) => [option.key, option]))
)
function getDefaultSortDirection(sortKey){
  return props.problemSortOptions.find((option) => option.key === sortKey)?.defaultDirection || 'asc'
}

function getSortButtonIndicator(sortKey){
  if (props.appliedSortKey !== sortKey) {
    return '↕'
  }

  return props.appliedSortDirection === 'asc' ? '↑' : '↓'
}

function getSortButtonTitle(option){
  const nextDirection = props.appliedSortKey === option.key
    ? (props.appliedSortDirection === 'asc' ? 'desc' : 'asc')
    : getDefaultSortDirection(option.key)
  const nextDirectionLabel = nextDirection === 'asc' ? '오름차순' : '내림차순'

  return `${option.label} ${nextDirectionLabel} 정렬`
}
</script>

<style scoped>
.problem-table {
  display: grid;
  border: 1px solid var(--line);
  border-radius: 24px;
  overflow: hidden;
  background: rgba(255, 255, 255, 0.78);
  box-shadow:
    0 18px 36px rgba(20, 33, 61, 0.06),
    inset 0 1px 0 rgba(255, 255, 255, 0.82);
}

.problem-table-head,
.problem-row {
  display: grid;
  grid-template-columns: 120px minmax(0, 1fr) 340px;
  gap: 1.15rem;
  align-items: center;
  padding: 1rem 1.25rem;
}

.problem-table-head {
  background: linear-gradient(180deg, rgba(255, 248, 240, 0.96), rgba(255, 255, 255, 0.92));
  color: var(--ink-soft);
  font-size: 0.82rem;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 0.08em;
}

.problem-table-head-button {
  display: inline-flex;
  align-items: center;
  justify-content: space-between;
  gap: 0.55rem;
  width: auto;
  max-width: 100%;
  min-height: 2rem;
  padding: 0.3rem 0.75rem;
  border: 1px solid rgba(20, 33, 61, 0.08);
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.72);
  color: inherit;
  font: inherit;
  font-weight: inherit;
  letter-spacing: inherit;
  cursor: pointer;
  transition:
    color 160ms ease,
    border-color 160ms ease,
    background 160ms ease,
    box-shadow 160ms ease;
}

.problem-table-head-button:hover {
  color: var(--ink-strong);
  border-color: rgba(217, 119, 6, 0.22);
  background: rgba(255, 255, 255, 0.96);
  box-shadow: 0 10px 20px rgba(20, 33, 61, 0.06);
}

.problem-table-head-button.is-active {
  color: var(--ink-strong);
  border-color: rgba(217, 119, 6, 0.28);
  background: rgba(255, 247, 237, 0.96);
  box-shadow:
    0 10px 22px rgba(20, 33, 61, 0.06),
    inset 0 1px 0 rgba(255, 255, 255, 0.78);
}

.problem-table-head-button--number {
  justify-self: start;
  justify-content: flex-start;
  text-align: left;
}

.problem-table-head-button-text {
  min-width: 0;
  white-space: nowrap;
}

.problem-table-head-button-indicator {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 1.25rem;
  min-height: 1.25rem;
  border-radius: 999px;
  background: rgba(20, 33, 61, 0.06);
  color: var(--ink-soft);
  font-size: 0.75rem;
  line-height: 1;
}

.problem-table-head-button.is-active .problem-table-head-button-indicator {
  background: rgba(217, 119, 6, 0.14);
  color: #b45309;
}

.problem-table-stats-head {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 0.9rem;
  align-items: center;
}

.problem-table-stats-head .problem-table-head-button {
  justify-self: end;
  justify-content: flex-end;
  text-align: right;
}

.problem-table-head + .problem-row,
.problem-row + .problem-row {
  border-top: 1px solid rgba(20, 33, 61, 0.08);
}

.problem-row {
  content-visibility: auto;
  contain-intrinsic-size: 88px;
}

.problem-row-link:hover {
  background: rgba(255, 247, 237, 0.7);
}

.problem-row-link {
  color: inherit;
  transition:
    background 160ms ease,
    color 160ms ease;
}

.problem-id {
  display: block;
  font-family: "Space Grotesk", "IBM Plex Sans", sans-serif;
  font-size: 1.05rem;
  padding-left: 0.75rem;
}

.problem-main {
  min-width: 0;
}

.problem-title-line {
  display: flex;
  gap: 0.6rem;
  align-items: center;
  flex-wrap: wrap;
}

.problem-main strong {
  display: block;
}

.problem-stats {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 0.9rem;
  align-items: center;
  justify-items: stretch;
  text-align: right;
}

.problem-stat-value,
.problem-rate-value {
  display: block;
  font-family: "Space Grotesk", "IBM Plex Sans", sans-serif;
  font-size: 1.05rem;
  padding-right: 0.7rem;
}

.problem-rate-value {
  min-width: 4.5rem;
}

@media (max-width: 900px) {
  .problem-table-head {
    display: none;
  }

  .problem-row,
  .problem-row-link {
    grid-template-columns: minmax(0, 1fr);
  }

  .problem-stats {
    grid-template-columns: repeat(3, minmax(0, 1fr));
    justify-items: start;
    text-align: left;
  }

  .problem-stat-value,
  .problem-rate-value {
    padding-right: 0;
  }

  .problem-id {
    padding-left: 0;
  }
}
</style>
