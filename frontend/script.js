// Example of handling message sending
document.getElementById("send-btn").addEventListener("click", () => {
    let messageInput = document.getElementById("message-input");
    let messageText = messageInput.value;

    if (messageText) {
        // Add new message to the message container
        let messageContainer = document.getElementById("message-container");
        
        let newMessage = document.createElement("div");
        newMessage.classList.add("message");
        
        let sender = document.createElement("span");
        sender.classList.add("sender");
        sender.textContent = "You:";

        let content = document.createElement("span");
        content.classList.add("content");
        content.textContent = messageText;

        newMessage.appendChild(sender);
        newMessage.appendChild(content);

        messageContainer.appendChild(newMessage);

        // Clear the input after sending
        messageInput.value = "";

        // Scroll to the bottom of the chat
        messageContainer.scrollTop = messageContainer.scrollHeight;
    }
});

// Example of handling switching between chat rooms
document.querySelectorAll(".chatroom").forEach((chatroom) => {
    chatroom.addEventListener("click", (e) => {
        document.getElementById("chatroom-name").textContent = e.target.textContent;
    });
});
