import pandas as pd
import numpy as np
import glob
import os

def pegar_csv_mais_recente():
    """Busca todos os CSVs na pasta e retorna o mais novo."""
    arquivos_csv = glob.glob("*.csv")
    
    if not arquivos_csv:
        return None
        
    # max() com os.path.getmtime descobre qual arquivo foi modificado por último
    arquivo_mais_recente = max(arquivos_csv, key=os.path.getmtime)
    return arquivo_mais_recente

def calibrar_zmpt(arquivo_csv, tensao_real_medida, coluna_adc='V', ganho_ads=0.125):
    try:
        df = pd.read_csv(arquivo_csv, sep=';')
    except Exception as e:
        print(f"Erro ao ler o arquivo: {e}")
        return

    df = df.dropna(subset=[coluna_adc])
    valores_adc = df[coluna_adc].values
    amostras = len(valores_adc)
    
    if amostras == 0:
        print("Erro: Nenhuma amostra válida encontrada na coluna.")
        return

    # 1. CÁLCULO DO OFFSET
    offset = np.mean(valores_adc)

    # 2. CÁLCULO DO TRUE RMS BRUTO
    media_quadrados = np.mean(valores_adc ** 2)
    variancia = media_quadrados - (offset ** 2)
    if variancia < 0: variancia = 0
    rms_bruto_adc = np.sqrt(variancia)

    # 3. CÁLCULO DO FATOR DE CONVERSÃO
    tensão_lida_ads_volts = (rms_bruto_adc * ganho_ads) / 1000.0
    fator_conversao_zmpt = tensao_real_medida / tensão_lida_ads_volts

    # EXIBINDO OS RESULTADOS
    print("\n" + "="*45)
    print(" ⚡ RESULTADOS DA CALIBRAÇÃO DO ZMPT101B ⚡")
    print("="*45)
    print(f"Total de Amostras  : {amostras}")
    print(f"Tensão Informada   : {tensao_real_medida} V")
    print("-" * 45)
    print(f"True RMS Bruto     : {rms_bruto_adc:.2f} bits")
    print(f"Offset Calculado   : {offset:.2f}")
    print(f"Fator de Conversão : {fator_conversao_zmpt:.2f}")
    print("="*45)
    
    print("\nCopie e cole isso no seu arquivo .h:")
    print(f"#define OFFSET {offset:.1f}f")
    print(f"#define FATOR_TENSAO_ADS {ganho_ads}f")
    print(f"#define FATOR_CONVERSAO_ZMPT {fator_conversao_zmpt:.2f}f\n")


# ==========================================
# CONFIGURAÇÃO DO TESTE
# ==========================================
# Coloque aqui o valor que você leu no multímetro na hora do teste
TENSAO_MULTIMETRO = 124.7

# MÁGICA: O Python encontra o arquivo sozinho!
arquivo_alvo = pegar_csv_mais_recente()

if arquivo_alvo:
    print(f"📁 Arquivo detectado automaticamente: {arquivo_alvo}")
    calibrar_zmpt(arquivo_alvo, TENSAO_MULTIMETRO, coluna_adc='V', ganho_ads=0.125)
else:
    print("❌ Nenhum arquivo .csv foi encontrado nesta pasta!")
    print("Certifique-se de que o CSV do Teleplot está na mesma pasta que este script.")