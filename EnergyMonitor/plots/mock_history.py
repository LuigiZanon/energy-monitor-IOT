import paho.mqtt.client as mqtt
import struct
import time
import random
from datetime import datetime

# Configurações do Broker
BROKER = "172.16.235.199" # Altere para o IP do seu servidor se necessário
PORT = 1883
TOPIC = "teste/esp"

# Aumentamos o lote para 1000 amostras (20 KB por envio) para ser rápido e seguro
BATCH_SIZE = 1000 

def on_connect(client, userdata, flags, reason_code, properties=None):
    if reason_code == 0:
        print("✅ Conectado ao Broker MQTT com sucesso!")
    else:
        print(f"❌ Falha ao conectar. Código: {reason_code}")

client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2, client_id="Simulador_Historico")
client.on_connect = on_connect

print(f"Tentando conectar ao broker {BROKER}:{PORT}...")
try:
    client.connect(BROKER, PORT, 60)
    client.loop_start()
except Exception as e:
    print(f"Erro de conexão: {e}")
    exit(1)

# Cálculo do tempo
# 48 horas * 60 minutos * 60 segundos = 172.800 amostras
TOTAL_AMOSTRAS = 48 * 3600
tempo_atual = int(time.time())
tempo_inicial = tempo_atual - TOTAL_AMOSTRAS

data_inicio_legivel = datetime.fromtimestamp(tempo_inicial).strftime('%Y-%m-%d %H:%M:%S')
data_fim_legivel = datetime.fromtimestamp(tempo_atual).strftime('%Y-%m-%d %H:%M:%S')

print(f"\nIniciando geração de {TOTAL_AMOSTRAS} amostras...")
print(f"Período: {data_inicio_legivel} até {data_fim_legivel}\n")

payload = bytearray()
amostras_no_lote = 0
lotes_enviados = 0

# Ciclo rápido (sem sleep) para processar os 2 dias imediatamente
for i in range(TOTAL_AMOSTRAS):
    # O timestamp avança 1 segundo a cada iteração
    timestamp = tempo_inicial + i
    
    # Geração dos valores simulados
    rms_zmpt1 = random.uniform(219.0, 221.5)
    rms_zmpt2 = random.uniform(219.0, 221.5)
    rms_sct1 = random.uniform(9.8, 10.5) 
    rms_sct2 = random.uniform(0.0, 0.1)
    
    # Empacotamento binário (20 bytes)
    amostra_binaria = struct.pack('<Iffff', timestamp, rms_sct1, rms_sct2, rms_zmpt1, rms_zmpt2)
    payload.extend(amostra_binaria)
    
    amostras_no_lote += 1
    
    # Quando o lote atinge 1000 amostras, envia para o MQTT
    if amostras_no_lote >= BATCH_SIZE:
        client.publish(TOPIC, payload)
        
        lotes_enviados += 1
        amostras_no_lote = 0
        payload = bytearray() # Limpa o buffer para o próximo lote
        
        # Imprime o progresso no terminal a cada 10 lotes (10.000 amostras)
        if lotes_enviados % 10 == 0:
            progresso = (lotes_enviados * BATCH_SIZE / TOTAL_AMOSTRAS) * 100
            print(f"Progresso: {lotes_enviados * BATCH_SIZE} amostras enviadas ({progresso:.1f}%)")

# Se sobrar alguma amostra no buffer final que não formou um lote completo (ex: 800 amostras)
if amostras_no_lote > 0:
    client.publish(TOPIC, payload)

# Aguarda 2 segundos para garantir que o Paho MQTT despacha os últimos pacotes de rede
time.sleep(2)

print("\n🚀 Injeção de dados históricos concluída com sucesso!")
client.loop_stop()
client.disconnect()