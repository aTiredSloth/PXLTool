<script lang="ts">
	import { onMount } from "svelte";
   import SceneComponent from "./components/SceneComponent.svelte";
	
	let Scenes = $state<Scene[]>([]);
	let NewSceneName = $state("NewScene");
	let CurrentScene = $state<Scene | null>();
	
	async function LoadScenes()
	{
		const response = await fetch('/api/scenes');
		let LoadedScenesJson = await response.json();
		let LoadedScenes: Scene[] = [];
		for (const item of LoadedScenesJson)
		{
			LoadedScenes.push(item);
		}

		Scenes = LoadedScenes;
	}

	onMount(()=>{LoadScenes()})

	async function CreateScene()
	{
		await fetch('/api/newscene/' + NewSceneName);
		LoadScenes();
	}
</script>


<div>
	<div>
		<input bind:value={NewSceneName}/>
		<button onclick={CreateScene}>
			Submit
		</button>
	</div>
	
	<div>
		{#each Scenes as SceneId}
			<li>
				<button onclick={()=>{CurrentScene = SceneId}}>
					{SceneId.Name}
				</button>
			</li>
		{/each}
	</div>

	<div>
		{#if CurrentScene}
			<SceneComponent Id={CurrentScene}/>
		{/if}
	</div>

</div>
