<script>
import { onMount } from "svelte";

onMount(async () => {
    await connectToServer();
});

async function connectToServer(){
    let response = await fetch(`/api/client`);
}

</script>

 