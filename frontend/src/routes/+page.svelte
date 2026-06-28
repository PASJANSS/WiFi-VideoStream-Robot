<script>
// @ts-nocheck
import { onMount } from "svelte";
let robotConnected = $state(false)

onMount(async () => {
    await connectToServer();
});


let ws, isWsOpened = false;

async function connectToServer(){
    ws = new WebSocket('ws://localhost:5173/ws/client')
    ws.onopen = () => {
        isWsOpened = true;
        console.log('połączono');
    }
    ws.onmessage = (event) => {
        const data = JSON.parse(event.data)
        robotConnected = data.robotConnected;
        console.log(robotConnected);
    }
    ws.onclose = () => {
        console.log('rozłączono');
        isWsOpened = false;
        setTimeout(connectToServer, 2000);
    }
    ws.onerror = (error) => {};
}

const inputChars = ['a', 's', 'd', 'w', 'q', 'e']
onMount(() => {
    window.addEventListener('keydown', async (event) => {
        if(inputChars.includes(event.key) && isWsOpened){
            console.log("sending");
            await ws.send((event.key).toString());
        }
    })
});

</script>

<div id="container">
    {#if !robotConnected}
        <h1>Robot is not connected</h1>
    {/if}

    {#if robotConnected}
        <img id="stream" src="/api/stream" alt="stream"/>
    {/if}
</div>

<style>
    :global(body) {
        font-size: 50px;
        font-family:Impact, Haettenschweiler, 'Arial Narrow Bold', sans-serif;
        background-color: antiquewhite;
        overflow: hidden;
    }
    #container{
        display: flex;
        justify-content: center;
        align-items: center;
        height: 100vh;
    }
    #stream{
        width: 80vw;
        height: 80vh;
        object-fit: contain;
    }
</style>