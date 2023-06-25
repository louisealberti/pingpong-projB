# 🏓 Ping Pong OS

O projeto PingPongOS visa construir, de forma incremental, um pequeno sistema
operacional didático. O sistema é construído inicialmente na forma de uma biblioteca de threads
cooperativas dentro de um processo do sistema operacional real (Linux, MacOS ou outro Unix)

### 📅 Datas Importantes
- 07/05/2023 - Entrega do Projeto A
- 25/06/2023 - Entrega do Projeto B
- 08/05/23 + 10/05/23 - Defesas do Projeto A
- 26/06/23 + 28/06/23 - Defesas do Projeto B

## 🅰️ Projeto A - Implementação de um Escalonador e Contabilização de Métricas



### 🎯 Objetivos e Requisitos
- [ ] Implementar um escalonador preemptivo baseado em prioridades com envelhecimento (***in progress***)
- [ ] Implementar a preempção por tempo (Sistema Multitarefa de Tempo Compartilhado)
- [ ] Fazer a contabilização de métricas sobre a execução das tarefas

### 📁 Arquivos a serem entregues:

* pa-ppos_data.h
* pa-ppos-core-aux.c
* pa-ppos-core-aux-modificacoes.txt

### ❗ IMPORTANTE:
1. Se for necessário criar novas funções, elas deverão ser incluidas no arquivo ppos-core-aux.c.
2. Os demais arquivos .c e .h, diferentes dos indicados acima, NÃO devem ser modificados.
3. Utilize as funções before_*() e after_*() conforme necessidade. Observe os comentários no arquivo ppos.h.
4. O arquivo "pa-ppos-core-aux-modificacoes.txt" é uma cópia do arquivo "ppos-core-aux.c", porém deve-se incluir apenas as funções modificadas/criadas do "ppos-core-aux.c". As demais funções funções before_*() e after_*() devem ser removidas, ou seja, remover todas as funções que não foram implementadas ou modificadas no projeto.


## 🅱️ Projeto B - Implementação de um Gerenciador de Discos
### Objetivos e Requisitos
- [ ] Implementar um gerenciador de disco (virtual)
- [ ] Implementar um escalonador de requisições de acesso ao disco (virtual)
