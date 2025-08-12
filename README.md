<h2>Описание проекта (Russian)</h2>
<p>Это моя дипломная работа. Проект был выполнен в условиях ограниченного времени, так как создание нейросети для распознавания речи заняло у меня очень много времени, поэтому код написан не идеально — упор делался на скорость разработки и выполнение задач, а не на чистоту и архитектуру кода.</p>

<p><strong>Суть работы:</strong></p>
<ul>
  <li>Программа позволяет распознавать субтитры из видеофайлов.</li>
  <li>После распознавания субтитры накладываются на видео.</li>
  <li>Также реализован монтаж видео по пользовательскому запросу.</li>
  <li>Монтаж осуществляется с помощью <code>ffmpeg</code> и нейронной сети LLM.</li>
  <li>В LLM отправляются субтитры с таймкодами и пользовательский промт (описание желаемого монтажа).</li>
  <li>В ответ нейронная сеть генерирует команды для <code>ffmpeg</code>, которые выполняются для создания итогового видео.</li>
</ul>

<p><em>Примечание:</em> Основное внимание уделялось скорости реализации и работоспособности проекта, а не оптимальной структуре или чистоте кода.</p>

<hr>

<h2>Project Description (English)</h2>
<p>This is my diploma project. The code was written in a hurry due to time constraints caused by challenges in training a speech recognition neural network, which took up a significant amount of time. Therefore, the code is not perfectly clean or well-structured.</p>

<p><strong>Project overview:</strong></p>
<ul>
  <li>The application enables subtitle recognition from video files.</li>
  <li>After recognition, subtitles are overlaid onto the video.</li>
  <li>Video editing based on user requests is also implemented.</li>
  <li>The editing process uses <code>ffmpeg</code> and an LLM neural network.</li>
  <li>The LLM receives subtitles with timecodes and a user prompt describing the desired edit.</li>
  <li>The neural network generates <code>ffmpeg</code> commands which are executed to produce the final video.</li>
</ul>

<p><em>Note:</em> The focus was on speed of development and functionality rather than code cleanliness or architecture.</p>
