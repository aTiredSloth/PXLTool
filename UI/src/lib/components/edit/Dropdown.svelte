<script>

	let {CurrentFilter, Options} = $props();
</script>

<select bind:value={CurrentFilter}>
	{#each Options as filter, i}
		<option value={filter.key}>{filter.display} - {i}</option>
	{/each}
</select>
<span>
	Selected key: {Options}
</span>
