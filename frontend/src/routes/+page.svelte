<script>
// @ts-nocheck
import { onMount } from 'svelte';

let robotConnected = $state(false);
let passwordPassed = $state(false);
let password = $state('');

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
            ws.send((event.key).toString());
        }
    })
});

async function submitButtonClicked(){
    if(password != "ILN367") return;
    passwordPassed = true;
    await connectToServer();
}

</script>

<div id = "container">
    {#if passwordPassed}
        {#if !robotConnected}
            <h1>Robot is not connected</h1>
        {/if}

        {#if robotConnected}
            <img id="stream" src="/api/stream" alt="stream"/>
        {/if}
    {/if}
    {#if !passwordPassed}
        <input bind:value={password}>
        <button onclick={submitButtonClicked}>Submit</button>
    {/if}
</div>

<style>
    :global(*) {
    box-sizing: border-box;
    margin: 0;
    padding: 0;
    }

    :global(body) {
    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
    background-color: #111;
    color: #eee;
    display: flex;
    justify-content: center;
    align-items: center;
    height: 100vh;
    overflow: hidden;
    }

    #container {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 16px;
    }

    h1 {
    font-size: 18px;
    font-weight: 400;
    color: #888;
    }

    input {
    padding: 10px 0;
    border: none;
    border-bottom: 2px solid #444;
    background: transparent;
    font-size: 15px;
    outline: none;
    color: #eee;
    width: 260px;
    text-align: center;
    }

    input:focus {
    border-bottom-color: #eee;
    }

    button {
    padding: 10px 32px;
    background-color: #eee;
    color: #111;
    border: none;
    border-radius: 4px;
    font-size: 14px;
    cursor: pointer;
    }

    button:hover {
    background-color: #ccc;
    }

    #stream {
    width: 100vw;
    height: 100vh;
    object-fit: contain;
    }
</style>