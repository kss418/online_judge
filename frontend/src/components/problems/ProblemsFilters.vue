<template>
  <div class="problem-filter-bar">
    <div
      v-if="showProblemStateFilters"
      class="problem-filter-group problem-state-filter-group"
    >
      <div class="problem-filter-chip-list">
        <button
          v-for="option in problemStateFilterOptions"
          :key="option.value || 'all'"
          type="button"
          class="ghost-button problem-filter-chip"
          :class="{ 'is-active': appliedStateFilter === option.value }"
          :disabled="isLoading"
          @click="$emit('applyStateFilter', option.value)"
        >
          {{ option.label }}
        </button>
      </div>
    </div>

    <div class="problem-filter-group problem-sort-chip-group">
      <span class="problem-filter-label">정렬</span>
      <div class="problem-filter-chip-list">
        <button
          v-for="option in problemSortOptions"
          :key="option.key"
          type="button"
          class="ghost-button problem-filter-chip"
          :class="{ 'is-active': appliedSortKey === option.key }"
          :disabled="isLoading"
          @click="$emit('cycleSort', option.key)"
        >
          {{ getSortButtonLabel(option) }}
        </button>
      </div>
    </div>
  </div>
</template>

<script setup>
const props = defineProps({
  showProblemStateFilters: {
    type: Boolean,
    default: false
  },
  isLoading: {
    type: Boolean,
    default: false
  },
  problemStateFilterOptions: {
    type: Array,
    required: true
  },
  problemSortOptions: {
    type: Array,
    required: true
  },
  appliedStateFilter: {
    type: String,
    default: ''
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

defineEmits([
  'applyStateFilter',
  'cycleSort'
])

function getSortButtonLabel(option){
  if (props.appliedSortKey !== option.key) {
    return option.label
  }

  return `${option.label} ${props.appliedSortDirection === 'asc' ? '↑' : '↓'}`
}
</script>

<style scoped>
.problem-filter-bar {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: flex-start;
  flex-wrap: wrap;
  padding-inline: 1.25rem;
}

.problem-filter-group {
  display: flex;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
}

.problem-filter-label {
  color: var(--ink-soft);
  font-size: 0.92rem;
  font-weight: 700;
  letter-spacing: 0.08em;
  text-transform: uppercase;
}

.problem-filter-chip-list {
  display: flex;
  gap: 0.6rem;
  align-items: center;
  flex-wrap: wrap;
}

.problem-filter-chip {
  min-height: 2.55rem;
  padding-inline: 1rem;
}

.problem-filter-chip.is-active {
  color: white;
  border-color: transparent;
  background: linear-gradient(135deg, #d97706, #ea580c);
  box-shadow: 0 16px 30px rgba(217, 119, 6, 0.22);
}

.problem-state-filter-group {
  gap: 0.6rem;
}

.problem-state-filter-group .problem-filter-chip-list {
  gap: 0.55rem;
}

.problem-state-filter-group .problem-filter-chip {
  min-height: 1.95rem;
  padding-inline: 0.95rem;
  font-size: 0.9rem;
}

.problem-sort-chip-group {
  display: none;
}

@media (max-width: 900px) {
  .problem-filter-bar {
    flex-direction: column;
    align-items: stretch;
  }

  .problem-filter-group {
    align-items: flex-start;
  }

  .problem-sort-chip-group {
    display: flex;
  }
}
</style>
