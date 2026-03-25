/**
 * 自定义模态框系统 - 替代原生alert和confirm
 * 提供统一的用户体验和现代化设计
 */

class ModalSystem {
    constructor() {
        this.init();
    }

    init() {
        // 创建模态框容器
        this.createModalContainer();
        // 绑定键盘事件
        this.bindEvents();
    }

    createModalContainer() {
        if (document.getElementById('modalContainer')) return;

        const container = document.createElement('div');
        container.id = 'modalContainer';
        container.innerHTML = `
            <!-- 通用模态框 -->
            <div id="genericModal" class="custom-modal">
                <div class="custom-modal-overlay"></div>
                <div class="custom-modal-content">
                    <div class="custom-modal-header">
                        <div class="custom-modal-title-wrap">
                            <span class="custom-modal-title-icon" id="modalTitleIcon"></span>
                            <h3 class="custom-modal-title" id="modalTitle">提示</h3>
                        </div>
                        <button class="custom-modal-close" id="genericModalClose">×</button>
                    </div>
                    <div class="custom-modal-body">
                        <div class="custom-modal-message" id="modalMessage"></div>
                        <div class="custom-modal-icon" id="modalIcon"></div>
                    </div>
                    <div class="custom-modal-footer" id="modalFooter">
                        <button class="btn btn-primary" id="genericModalOk">确定</button>
                    </div>
                </div>
            </div>

            <!-- 确认模态框 -->
            <div id="confirmModal" class="custom-modal">
                <div class="custom-modal-overlay"></div>
                <div class="custom-modal-content">
                    <div class="custom-modal-header">
                        <div class="custom-modal-title-wrap">
                            <span class="custom-modal-title-icon warning" id="confirmTitleIcon">!</span>
                            <h3 class="custom-modal-title" id="confirmTitle">确认操作</h3>
                        </div>
                        <button class="custom-modal-close" id="confirmModalClose">×</button>
                    </div>
                    <div class="custom-modal-body">
                        <div class="custom-modal-message" id="confirmMessage"></div>
                    </div>
                    <div class="custom-modal-footer">
                        <button class="btn btn-secondary" id="confirmModalCancel">取消</button>
                        <button class="btn btn-danger" id="confirmBtn">确定</button>
                    </div>
                </div>
            </div>

            <!-- 通知提示框 -->
            <div id="notification" class="custom-notification">
                <div class="notification-content">
                    <span class="notification-icon" id="notificationIcon"></span>
                    <div class="notification-copy">
                        <span class="notification-title" id="notificationTitle"></span>
                        <span class="notification-message" id="notificationMessage"></span>
                    </div>
                </div>
            </div>
        `;
        document.body.appendChild(container);
        
        // 绑定事件监听器
        this.bindModalEvents();
    }

    bindModalEvents() {
        // 通用模态框关闭按钮
        const genericModalClose = document.getElementById('genericModalClose');
        if (genericModalClose) {
            genericModalClose.addEventListener('click', () => this.close());
        }

        // 通用模态框确定按钮
        const genericModalOk = document.getElementById('genericModalOk');
        if (genericModalOk) {
            genericModalOk.addEventListener('click', () => this.close());
        }

        // 通用模态框遮罩层
        const genericOverlay = document.querySelector('#genericModal .custom-modal-overlay');
        if (genericOverlay) {
            genericOverlay.addEventListener('click', () => this.close());
        }

        // 确认模态框关闭按钮
        const confirmModalClose = document.getElementById('confirmModalClose');
        if (confirmModalClose) {
            confirmModalClose.addEventListener('click', () => this.confirmCancel());
        }

        // 确认模态框取消按钮
        const confirmModalCancel = document.getElementById('confirmModalCancel');
        if (confirmModalCancel) {
            confirmModalCancel.addEventListener('click', () => this.confirmCancel());
        }

        // 确认模态框遮罩层
        const confirmOverlay = document.querySelector('#confirmModal .custom-modal-overlay');
        if (confirmOverlay) {
            confirmOverlay.addEventListener('click', () => this.confirmCancel());
        }
    }

    bindEvents() {
        // ESC键关闭模态框
        document.addEventListener('keydown', (e) => {
            if (e.key === 'Escape') {
                this.close();
                this.confirmCancel();
            }
        });

        // Enter键确认
        document.addEventListener('keydown', (e) => {
            if (e.key === 'Enter') {
                const confirmModal = document.getElementById('confirmModal');
                if (confirmModal.classList.contains('show')) {
                    const confirmBtn = document.getElementById('confirmBtn');
                    if (confirmBtn) confirmBtn.click();
                }
            }
        });
    }

    /**
     * 显示提示框（替代alert）
     * @param {string} message - 提示消息
     * @param {string} title - 标题（可选）
     * @param {string} type - 类型：info, success, warning, error
     * @param {Function} callback - 回调函数（可选）
     */
    alert(message, title = '提示', type = 'info', callback = null) {
        const modal = document.getElementById('genericModal');
        const modalTitle = document.getElementById('modalTitle');
        const modalTitleIcon = document.getElementById('modalTitleIcon');
        const modalMessage = document.getElementById('modalMessage');
        const modalIcon = document.getElementById('modalIcon');
        const modalFooter = document.getElementById('modalFooter');

        modal.classList.add('alert-mode');
        modal.classList.remove('detail-mode');

        // 设置内容
        modalTitle.textContent = title;
        modalMessage.textContent = message;
        modalTitleIcon.className = 'custom-modal-title-icon';

        // 设置图标和样式
        modalIcon.className = 'custom-modal-icon';
        modalFooter.innerHTML = '';

        switch (type) {
            case 'success':
                modalTitleIcon.textContent = '✓';
                modalTitleIcon.classList.add('success');
                modalIcon.innerHTML = 'OK';
                modalIcon.classList.add('success');
                this.addModalButton(modalFooter, '确定', 'btn btn-success', () => {
                    this.close();
                    if (callback) callback();
                });
                break;
            case 'warning':
                modalTitleIcon.textContent = '!';
                modalTitleIcon.classList.add('warning');
                modalIcon.innerHTML = '!';
                modalIcon.classList.add('warning');
                this.addModalButton(modalFooter, '确定', 'btn btn-warning', () => {
                    this.close();
                    if (callback) callback();
                });
                break;
            case 'error':
                modalTitleIcon.textContent = '×';
                modalTitleIcon.classList.add('error');
                modalIcon.innerHTML = 'X';
                modalIcon.classList.add('error');
                this.addModalButton(modalFooter, '确定', 'btn btn-danger', () => {
                    this.close();
                    if (callback) callback();
                });
                break;
            default: // info
                modalTitleIcon.textContent = 'i';
                modalTitleIcon.classList.add('info');
                modalIcon.innerHTML = 'i';
                modalIcon.classList.add('info');
                this.addModalButton(modalFooter, '确定', 'btn btn-primary', () => {
                    this.close();
                    if (callback) callback();
                });
        }

        // 显示模态框
        this.showModal(modal);
    }

    detail(title = '详情', items = [], actions = []) {
        const modal = document.getElementById('genericModal');
        const modalTitle = document.getElementById('modalTitle');
        const modalTitleIcon = document.getElementById('modalTitleIcon');
        const modalMessage = document.getElementById('modalMessage');
        const modalIcon = document.getElementById('modalIcon');
        const modalFooter = document.getElementById('modalFooter');

        modal.classList.remove('alert-mode');
        modal.classList.add('detail-mode');
        modalTitle.textContent = title;
        modalTitleIcon.className = 'custom-modal-title-icon hidden';
        modalTitleIcon.textContent = '';
        modalIcon.className = 'custom-modal-icon hidden';
        modalIcon.innerHTML = '';

        const rowsHtml = items
            .map((item) => {
                const label = item.label || '';
                const value = item.value == null || item.value === '' ? '-' : item.value;
                return `
                    <div class="detail-row">
                        <div class="detail-label">${label}</div>
                        <div class="detail-value">${value}</div>
                    </div>
                `;
            })
            .join('');

        modalMessage.innerHTML = `<div class="detail-sheet">${rowsHtml}</div>`;

        modalFooter.innerHTML = '';
        if (Array.isArray(actions) && actions.length > 0) {
            actions.forEach((action) => {
                this.addModalButton(
                    modalFooter,
                    action.label || '确定',
                    action.className || 'btn btn-primary',
                    () => {
                        if (action.closeOnClick !== false) {
                            this.close();
                        }
                        if (typeof action.onClick === 'function') {
                            action.onClick();
                        }
                    }
                );
            });
        } else {
            this.addModalButton(modalFooter, '关闭', 'btn btn-primary', () => this.close());
        }

        this.showModal(modal);
    }

    /**
     * 显示确认框（替代confirm）
     * @param {string} message - 确认消息
     * @param {string} title - 标题（可选）
     * @param {Function} onConfirm - 确认回调
     * @param {Function} onCancel - 取消回调（可选）
     */
    confirm(message, title = '确认操作', onConfirm = null, onCancel = null) {
        const modal = document.getElementById('confirmModal');
        const confirmTitle = document.getElementById('confirmTitle');
        const confirmMessage = document.getElementById('confirmMessage');
        const confirmBtn = document.getElementById('confirmBtn');

        // 设置内容
        confirmTitle.textContent = title;
        confirmMessage.textContent = message;

        // 绑定确认按钮事件
        confirmBtn.onclick = () => {
            this.confirmClose();
            if (onConfirm) onConfirm();
        };

        // 保存取消回调
        this.currentCancelCallback = onCancel;

        // 显示模态框
        this.showModal(modal);
    }

    /**
     * 显示通知提示
     * @param {string} message - 通知消息
     * @param {string} type - 类型：success, warning, error, info
     * @param {number} duration - 显示时长（毫秒），0表示不自动关闭
     */
    notification(message, type = 'info', duration = 3000) {
        const notification = document.getElementById('notification');
        const notificationIcon = document.getElementById('notificationIcon');
        const notificationTitle = document.getElementById('notificationTitle');
        const notificationMessage = document.getElementById('notificationMessage');

        // 设置内容
        notificationMessage.textContent = message;

        // 设置图标和样式
        notification.className = 'custom-notification';
        notificationIcon.className = 'notification-icon';

        switch (type) {
            case 'success':
                notification.classList.add('success');
                notificationTitle.textContent = '成功';
                notificationIcon.innerHTML = '✓';
                break;
            case 'warning':
                notification.classList.add('warning');
                notificationTitle.textContent = '提醒';
                notificationIcon.innerHTML = '!';
                break;
            case 'error':
                notification.classList.add('error');
                notificationTitle.textContent = '错误';
                notificationIcon.innerHTML = '×';
                break;
            default: // info
                notification.classList.add('info');
                notificationTitle.textContent = '提示';
                notificationIcon.innerHTML = 'i';
        }

        // 显示通知
        notification.classList.add('show');

        // 自动关闭
        if (duration > 0) {
            setTimeout(() => {
                this.closeNotification();
            }, duration);
        }
    }

    /**
     * 显示加载中模态框
     * @param {string} message - 加载消息
     * @param {string} title - 标题
     */
    loading(message = '正在处理...', title = '请稍候') {
        const modal = document.getElementById('genericModal');
        const modalTitle = document.getElementById('modalTitle');
        const modalMessage = document.getElementById('modalMessage');
        const modalIcon = document.getElementById('modalIcon');
        const modalFooter = document.getElementById('modalFooter');

        // 设置内容
        modalTitle.textContent = title;
        modalMessage.textContent = message;

        // 设置加载图标
        modalIcon.className = 'custom-modal-icon loading';
        modalIcon.innerHTML = '<div class="loading-spinner"></div>';

        // 隐藏按钮
        modalFooter.innerHTML = '';

        // 显示模态框
        this.showModal(modal);

        // 返回关闭函数
        return () => this.close();
    }

    /**
     * 添加模态框按钮
     * @param {HTMLElement} container - 按钮容器
     * @param {string} text - 按钮文本
     * @param {string} className - CSS类名
     * @param {Function} onClick - 点击事件
     */
    addModalButton(container, text, className, onClick) {
        const button = document.createElement('button');
        button.textContent = text;
        button.className = className;
        button.onclick = onClick;
        container.appendChild(button);
    }

    /**
     * 显示模态框
     * @param {HTMLElement} modal - 模态框元素
     */
    showModal(modal) {
        modal.classList.add('show');
        document.body.style.overflow = 'hidden';
        
        // 焦点管理
        setTimeout(() => {
            const firstButton = modal.querySelector('button');
            if (firstButton) firstButton.focus();
        }, 100);
    }

    /**
     * 关闭通用模态框
     */
    close() {
        const modal = document.getElementById('genericModal');
        modal.classList.remove('show');
        document.body.style.overflow = 'auto';
    }

    /**
     * 关闭确认模态框
     */
    confirmClose() {
        const modal = document.getElementById('confirmModal');
        modal.classList.remove('show');
        document.body.style.overflow = 'auto';
        this.currentCancelCallback = null;
    }

    /**
     * 取消确认操作
     */
    confirmCancel() {
        this.confirmClose();
        if (this.currentCancelCallback) {
            this.currentCancelCallback();
            this.currentCancelCallback = null;
        }
    }

    /**
     * 关闭通知
     */
    closeNotification() {
        const notification = document.getElementById('notification');
        notification.classList.remove('show');
    }
}

// 确保在DOM加载完成后初始化
function initializeModalSystem() {
    try {
        if (!window.ModalSystem) {
            console.log('正在初始化模态框系统...');
            window.ModalSystem = new ModalSystem();
            
            // 提供全局便捷函数
            window.showAlert = (message, title, type, callback) => {
                try {
                    window.ModalSystem.alert(message, title, type, callback);
                } catch (error) {
                    console.error('showAlert 错误:', error);
                    if (callback) callback();
                }
            };
            
            window.showConfirm = (message, title, onConfirm, onCancel) => {
                try {
                    window.ModalSystem.confirm(message, title, onConfirm, onCancel);
                } catch (error) {
                    console.error('showConfirm 错误:', error);
                    if (onCancel) onCancel();
                }
            };
            
            window.showNotification = (message, type, duration) => {
                try {
                    window.ModalSystem.notification(message, type, duration);
                } catch (error) {
                    console.error('showNotification 错误:', error);
                    // 降级到console.log
                    console.log(`[${type.toUpperCase()}] ${message}`);
                }
            };
            
            window.showLoading = (message, title) => {
                try {
                    return window.ModalSystem.loading(message, title);
                } catch (error) {
                    console.error('showLoading 错误:', error);
                    // 降级到console.log
                    console.log(`[LOADING] ${title}: ${message}`);
                    return () => {};
                }
            };

            window.showDetail = (title, items, actions) => {
                try {
                    window.ModalSystem.detail(title, items, actions);
                } catch (error) {
                    console.error('showDetail 错误:', error);
                }
            };
             
            console.log('模态框系统初始化成功');
        }
    } catch (error) {
        console.error('模态框系统初始化失败:', error);
    }
}

// 多重初始化策略
function tryInitialize() {
    if (window.showAlert && window.showConfirm && window.showNotification) {
        console.log('模态框函数已存在');
        return;
    }
    
    initializeModalSystem();
}

// 如果DOM已经加载完成，立即初始化
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', tryInitialize);
} else {
    // DOM已经加载完成，延迟一点时间确保其他脚本也加载完成
    setTimeout(tryInitialize, 100);
}

// 最后的保险措施：确保在window加载完成后可用
window.addEventListener('load', () => {
    setTimeout(tryInitialize, 200);
});
