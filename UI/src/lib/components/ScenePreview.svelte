<script lang="ts">

	let sceneId = $props();
	let image = $state<string | null>(null);
	export async function reload()
	{
  		const response = await fetch('/api/scene/'+ sceneId +'/render');
    	const blob = await response.blob();
    	image = URL.createObjectURL(blob);
	}
</script>

<img src={image} alt="Render">
