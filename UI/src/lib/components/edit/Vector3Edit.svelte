<script lang="ts">
  // Define the vector object state
  let vector = $props();
</script>

<div class="vector-input">
  <label>
    X:
    <input type="number" bind:value={vector.x} step="0.1" />
  </label>
  <label>
    Y:
    <input type="number" bind:value={vector.y} step="0.1" />
  </label>
  <label>
    Z:
    <input type="number" bind:value={vector.z} step="0.1" />
  </label>
</div>
