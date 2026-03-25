<template>
  <section class="page-grid single-column">
    <article class="panel">
      <div class="panel-header">
        <div>
          <p class="panel-kicker">problems</p>
          <h3>문제 화면 뼈대</h3>
        </div>
        <StatusBadge label="Scaffolded" tone="success" />
      </div>

      <p class="lead-copy">
        이 페이지는 문제 목록, 문제 상세, 관리 기능을 붙이기 위한 자리입니다.
        지금은 구조만 만들어 둔 상태라서 다음 작업에서 API 연결과 테이블 컴포넌트를 얹으면 됩니다.
      </p>

      <div class="todo-list">
        <div class="todo-item">
          <strong>목록 화면</strong>
          <p>문제 번호, 제목, 정답률, 공개 여부 같은 컬럼으로 시작하면 무난합니다.</p>
        </div>
        <div class="todo-item">
          <strong>상세 화면</strong>
          <p>문제 본문, 샘플, 테스트케이스, 제한값을 카드 단위로 나누면 관리가 쉽습니다.</p>
        </div>
        <div class="todo-item">
          <strong>관리자 액션</strong>
          <p>생성, 수정, 삭제는 나중에 모달보다는 별도 폼 화면으로 시작하는 편이 덜 복잡합니다.</p>
        </div>
      </div>
    </article>
  </section>
</template>

<script setup>
import StatusBadge from '@/components/StatusBadge.vue'
</script>
