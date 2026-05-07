import paho.mqtt.client as mqtt
import struct
import time
import random

# Configurações do Broker (Iguais as do ESP32)
BROKER = "127.0.0.1"
PORT = 1883
TOPIC = "teste/esp"
BATCH_SIZE = 10

def on_connect(client, userdata, flags, reason_code, properties=None):
    if reason_code == 0:
        print("✅ Simulador conectado ao Broker MQTT com sucesso!")
    else:
        print(f"❌ Falha ao conectar. Código: {reason_code}")

# Inicializa o cliente MQTT (Usando API v2 conforme seu script de recepção)
client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2, client_id="Simulador_ESP32")
client.on_connect = on_connect

print(f"Tentando conectar ao broker {BROKER}:{PORT}...")
try:
    client.connect(BROKER, PORT, 60)
    client.loop_start() # Inicia a thread de rede do MQTT
except Exception as e:
    print(f"Erro de conexão: {e}")
    exit(1)

try:
    while True:
        payload = bytearray()
        print(f"\nColetando lote de {BATCH_SIZE} amostras...")
        
        for i in range(BATCH_SIZE):
            # 1. Simulando a sincronização NTP do ESP32 (Unix Timestamp em segundos)
            # A função time.time() devolve os segundos absolutos (ex: 1777919796)
            timestamp = int(time.time()) & 0xFFFFFFFF
            
            # 2. Gerando valores simulados realistas
            # ZMPT: Tensões variando levemente em torno de 220V
            rms_zmpt1 = random.uniform(219.0, 221.5)
            rms_zmpt2 = random.uniform(219.0, 221.5)
            
            # SCT: Correntes variando (Simulando um equipamento de 10A ligado na fase 1 e nada na fase 2)
            rms_sct1 = random.uniform(9.8, 10.5) 
            rms_sct2 = random.uniform(0.0, 0.1)  # Ruído de fundo (perto de 0A)
            
            # 3. Empacotamento Binário
            # '<'     : Little Endian (Padrão do processador do ESP32)
            # 'I'     : uint32_t (4 bytes para o timestamp)
            # 'ffff'  : 4x float de precisão simples (4 bytes cada)
            # Total   : 20 bytes perfeitos (sem padding)
            amostra_binaria = struct.pack('<Iffff', timestamp, rms_sct1, rms_sct2, rms_zmpt1, rms_zmpt2)
            
            # Adiciona ao buffer do lote
            payload.extend(amostra_binaria)
            
            # Print de progresso no terminal
            print(f"  [Amostra {i+1}/{BATCH_SIZE}] TS: {timestamp} | V1: {rms_zmpt1:.1f}V | I1: {rms_sct1:.2f}A")
            
            # Aguarda (Atenção: como colocou 0.1s, vai gerar várias amostras no mesmo "segundo" do Unix)
            time.sleep(0.1)
        
        # 4. Envia o pacote completo (Ex: 10 amostras * 20 bytes = 200 bytes)
        client.publish(TOPIC, payload)
        print(f"🚀 Lote binário publicado! ({len(payload)} bytes enviados)")

except KeyboardInterrupt:
    print("\n🛑 Simulador encerrado pelo utilizador.")
    client.loop_stop()
    client.disconnect()