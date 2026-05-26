<script lang="ts">
    import { onMount } from "svelte";

	let Scenes = $state<string[]>([]);
	let NewSceneName = $state("NewScene");
	
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
	<div>
		<input value={NewSceneName}"/>
		<button onclick={CreateScene}>
			
		</button>
	</div>
	
	<div>
		{#each Scenes as SceneId}
			<button>
				${SceneId}
			</button>
		{/each}
	</div>
	
	
</div>
