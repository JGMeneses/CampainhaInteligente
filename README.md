# Projeto _Campainha Inteligente_
Este projeto é parte do trabalho final da disciplina de Tópicos Especiais em Sistemas Embarcados e visa resolver o problema de atrasos no atendimento ao portão dos residentes da Escola Agrícola de Jundiaí. A solução propõe uma campainha inteligente que notifica o porteiro automaticamente via Telegram quando alguém aproxima a mão da capainha, melhorando o tempo de resposta e a eficiência no atendimento.

## Problemática
Na residência universitária da EAJ/UFRN, os estudantes frequentemente se deparam com o portão fechado e a ausência do vigia. Isso faz com que os residentes tenham que chamar a atenção batendo no portão ou gritando, o que não só é incômodo, mas também perigoso, especialmente à noite. Resolver esse problema é crucial para garantir a segurança e o bem-estar dos residentes, além de melhorar a qualidade de vida no ambiente universitário.

## Solução
O projeto Campainha Inteligente visa substituir o método atual de chamar o porteiro na portaria da residência, que muitas vezes envolve barulho e demora. O sistema será composto por um sensor ultrassônico acoplado a um ESP32 que, ao detectar a aproximação de alguém, enviará uma mensagem ao porteiro, permitindo uma resposta mais rápida e silenciosa.

![image](https://github.com/user-attachments/assets/cf648a41-be4f-4a2d-a8bd-2526536841e6)

## Link para documento de requisitos
https://docs.google.com/document/d/1Qcvy0saoRv7pizZXeQtmrtq20TMVESwadkf_h6MvpFQ/edit?usp=sharing

## Requisitos
| Funcionais | Não Funcionais |
|----------|----------|
| [RF001] - Detectar aproximação de pessoas   | [NF001] - Utilizar ESP32   |
| [RF002] - Enviar mensagem para Telegram    | [NF002] - Fazer comunicação com transmissão de dados   |
| [RF003] - Confirmação visual ao enviar mensagem para o porteiro | [NF003] - A mensagem tem que ser enviada em tempo real |
| [RF004] - Confirmação visual após resposta do porteiro | [NF004] - Portabilidade |

## Regras de Negócio
### RN001 - Notificação
O porteiro deve ser notificado sempre que uma pessoa se aproximar do portão e o sistema detectar sua presença. A notificação deve ser enviada via Telegram.
### RN002 - Comunicação Visual
O sistema deve acender uma luz e enviar uma mensagem para o porteiro. Depois que o porteiro responder à mensagem, uma segunda luz deve ser acionada. A resposta do porteiro indica que ele tomou conhecimento da chegada do visitante e está a caminho.
### RN003 - Tempo de Resposta
 O porteiro deve responder à notificação em até 5 minutos, caso contrário, o sistema pode emitir um aviso sonoro ou repetir o envio da notificação.

## Descrição dos usuários
### Moradores e visitantes
Interagem com o sistema ao se aproximar a mão do sensor. Eles receberão
feedback visual (LED) de que o porteiro foi notificado.

### Porteiro
Responsável por monitorar as mensagens recebidas no Telegram  e tomar as ações
necessárias, como abrir o portão.

## Diagrama de visão geral do projeto
![DIAGRAMA DE VISÃO GERAL](https://github.com/user-attachments/assets/8056f579-8e20-41eb-bf1b-125f5f70ea58)

## Diagrama de sequência
![image](https://github.com/user-attachments/assets/f9eb7c4b-25e6-472a-9001-3f9256e3278e)

## Diagrama de implantação
![image](https://github.com/user-attachments/assets/cab3aca9-e391-445f-8e1b-9fcb33c16b8c)

