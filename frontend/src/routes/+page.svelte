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
            await ws.send(JSON.stringify({key: event.key}));
        }
    })
});

</script>

{#if !robotConnected}
    Robot is not connected
{/if}

{#if robotConnected}
    <img src="/api/stream" alt="stream"/>
{/if}