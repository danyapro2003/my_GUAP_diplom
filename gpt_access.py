import sys
import asyncio
import g4f
import g4f.Provider as Provider
import subprocess
import functools
print = functools.partial(print, flush=True)

# Получаем путь к файлу с текстом
if len(sys.argv) < 2:
    print("Ошибка: не передан путь к файлу с запросом.")
    sys.exit(1)

file_path = sys.argv[1]

try:
    with open(file_path, 'r', encoding='utf-8') as f:
        user_prompt = f.read()
except Exception as e:
    print(f"Ошибка чтения файла: {e}")
    sys.exit(1)

PROVIDERS = [
    Provider.Blackbox,
    Provider.Yqcloud
]

sysprompt = r"""Ты — умный помощник по видеомонтажу. На вход ты получаешь:
1. Субтитры в формате .srt (со временем и текстом).
2. Пожелания пользователя (например: «сделай нарезку из самых громких сцен»).

На выходе ты должен сгенерировать одну команду ffmpeg, которая вырежет нужные фрагменты, добавит эффекты (slow motion, черно-белое и т. д.) и прочие манипуляции с видео и затем сохранят результат в файл ai_rendered_video.mp4.

Формат вывода — только ffmpeg-команды, без пояснений.

Пример:

Пожелание: «Замедли момент с криком и сделай черно-белым»
Субтитры:
00:00:15,000 --> 00:00:20,000
Он кричит от ужаса.

Ответ:
ffmpeg -y -i input.mp4 -ss 00:00:15 -to 00:00:20 -vf setpts=2.0*PTS,format=gray output.mp4
Конец примера.
ВАЖНО:
НЕ ЗАБЫВАЙ ЕЩЕ ДОБАВЛЯТЬ ФЛАГ -y
Пиши только команды вида:
ffmpeg -y -i input.mp4 -vf setpts=0.5*PTS -af atempo=2.0 output.mp4
Никаких кавычек, никаких filter_complex, один выводной файл — ai_rendered_video.mp4
ПИШИ В ОДНУ СТРОЧКУ НИКОГДА НЕ ОБОРАЧИВАЙ ПАРАМЕТРЫ В КАВЫЧКИ
ВИДЕО СОХРАНЯЙ В ФАЙЛ ai_rendered_video.mp4
ИСХОДНЫЙ ФАЙЛ ВИДЕО НАХОДИТСЯ ПО СЛЕДУЮЩЕМУ ПУТИ, ЕГО И ИСПОЛЬЗУЙ ДЛЯ НАПИСАНИЯ СКРИПТА ffmpeg: """

async def ask_gpt(full_prompt: str) -> str:
    last_exception = None
    full_prompt = sysprompt + full_prompt
    print("PytHOOOOON")
    for prov in PROVIDERS:
        try:
            print(f'Send response to gpt {prov}')
            response = await g4f.ChatCompletion.create_async(
                model='gpt-4o',
                provider=prov,
                messages=[
                    {"role": "user", "content": full_prompt}
                ]
            )
            print('wait chego-to')
            with open("prompt.txt", "w", encoding="utf-8") as f:
                f.write(full_prompt)
            return response
        except Exception as e:
            last_exception = e
    return f"Ошибка: {last_exception}"

async def run_pipeline(user_prompt):
    attempt = 1
    while attempt <= 10:
        print(f"\nПопытка #{attempt}...\n")
        gpt_response = await ask_gpt(user_prompt)
        print("GPT ответ:\n", gpt_response)
        gpt_response = gpt_response.replace('"','')

        if not gpt_response.startswith("ffmpeg "):
            print("Ошибка: GPT не вернул корректную команду ffmpeg.")
            break

        try:
            command = gpt_response.strip()
            command_parts = command.split()
            ffmpeg_path = command_parts[0]
            args = command_parts[1:]

            print(">>> Запускаем ffmpeg:\n", command)
            process = subprocess.run([ffmpeg_path] + args, capture_output=True, text=True)

            print("FFmpeg STDOUT:\n", process.stdout)
            print("FFmpeg STDERR:\n", process.stderr)

            if process.returncode != 0:
                print(f"FFmpeg завершился с ошибкой: {process.returncode}")
                if attempt == 1:
                    # Добавляем ошибку в prompt и повторяем
                    user_prompt += f"\n\nОшибка при выполнении ffmpeg:\n{process.stderr.strip()}"
                    attempt += 1
                    continue
                else:
                    break
            else:
                print("FFmpeg успешно завершён.")
                break
        except Exception as e:
            print(f"Ошибка при выполнении ffmpeg: {e}")
            break

if __name__ == "__main__":
    asyncio.run(run_pipeline(user_prompt))
