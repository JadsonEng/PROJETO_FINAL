# Jadson de Jesus Santos - Projeto Final

## 📚 Descrição

Este projeto implementa um sistema de monitoramento de motor utilizando um microcontrolador. Ele simula falhas no estator e no rotor por meio de botões, e utiliza um potenciômetro para simular variações indesejadas de tensão que podem causar sobretensão ou subtensão. O status do motor, bem como os estados do estator e do rotor, é exibido em um display OLED. Além disso, o LED verde, que representa o motor, pode ser ligado ou desligado através do botão do joystick. Essa integração de sensores, atuadores e interface visual demonstra de forma prática conceitos de sistemas embarcados e monitoramento em tempo real.

## 🎯 Objetivo

- Fixar conhecimentos obtidos ao longo do cursos Embarcatech;
- Aplicar o que foi aprendido na criação de um projeto autoral; 
- Implementar um sistema embarcado que simule o monitoramento do estado operacional de motores de indução trifásicos;
- Propor uma solução para um problema e validar o projeto por meio de uma simulação na BitDogLab.

## 📑 Requisitos

- Plataforma de Desenvolvimento BitDogLab.
- Visual Studio Code;
- Extensões C/C++, CMAKE e Raspberry Pi Pico no VSCode;

## 📋 Funcionamento

- O display OLED indica inicialmente uma mensagem de "Motor Desligado";
- O Botão do Joystick liga/desliga o LED verde que indica o estado do motor (ligado/desligado);
- Se o LED verde ligado, um menu indicando o Status do Motor é exibido no display OLED;
- O Botão A liga/desliga o LED Vermelho e exibe no menu o estado do Estator (normal/falha);
- O Botão B liga/desliga o LED Azul e exibe no menu o estado do Rotor (normal/falha);
- O Eixo X do Joystick simula a tensão no motor, exibindo o estado no display, "Sobretensao" se a tensão for alta, "Subtensao" se a tensão for baixa e "Normal" se a tensão for adequada.


## ▶️ Link do Vídeo: https://youtu.be/vYeZpim3N0s
