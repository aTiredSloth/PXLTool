<script lang="ts">
    import { onMount } from "svelte";

	let Scenes = $state<string[]>([]);
	
	async function LoadScenes()
	{
		const response = await fetch('/api/scenes');
		Scenes = await response.json();
	}

	onMount(()=>{LoadScenes()})

	async function CreateScene()
	{
		const response = await fetch('/api/newscene');
		const text = await response.text();

		Scenes = Scenes.concat(text);
	}
</script>


<div>
	<button onclick={CreateScene}>
		New Scene
	</button>
	
	<div>
		{#each Scenes as SceneId}
			<button>
				${SceneId}
			</button>
		{/each}
	</div>
	
	
</div>
